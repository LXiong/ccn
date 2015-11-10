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
//

////////////////////////////////////////////////////////////////////////////
//
// File      :  access_list.c
// Objectives:  Access List implementation based on CISCO IOS.
//              Security feature implemented at the routers and the
//              gateway for packet filtering.
// References:  CISCO documentation
//              http://www.cisco.com/univercd/cc/td/doc/product/software/
//                  ios122/122cgcr/fipras_r/1rfip1.htm#xtocid2
//              For static and Standard Access List refer:
//              http://www.cisco.com/univercd/cc/td/doc/product/software/
//                  ios113ed/113ed_cr/np1_c/1cip.htm#xtocid847415
//              For Reflexive Access Lists refer:
//              http://www.cisco.com/univercd/cc/td/doc/product/software/
//                  ios120/12cgcr/secur_c/scprt3/screflex.htm#13069
// Date:        04/08/2002: Standard, Extended Access list, IOS functions
//              06/23/2002: Reflex Access List
//              01/31/2003: Function, 'AccessListVerifyRouteMap(...)' added
//                  for route map functionality.
////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "api.h"
#include "qualnet_error.h"
#include "network_ip.h"
#include "transport_tcp_hdr.h"
#include "transport_udp.h"
#include "network_access_list.h"
#include "message.h"
#include "network_icmp.h"
#include "route_parse_util.h"


////////////////////////////////////////////////////////////////////////////
// For debugging, set 0 for debug OFF, 1 for debug ON

#define ACCESS_LIST_REFLEX_DEBUG 0
////////////////////////////////////////////////////////////////////////////



//--------------------------------------------------------------------------
// FUNCTION  AccessListCreateTemplate
// PURPOSE   Allocate memory for a new access list
// ARGUMENTS none
// RETURN    Pointer to created access list
//--------------------------------------------------------------------------
static
AccessList* AccessListCreateTemplate()
{
    AccessList* newAccessList = (AccessList*) RtParseMallocAndSet(sizeof(
                        AccessList));

    newAccessList->filterType = ACCESS_LIST_DENY;
    newAccessList->protocol = 0;
    newAccessList->srcAddr = 0;
    newAccessList->srcMaskAddr = 0;
    newAccessList->destAddr = 0;
    newAccessList->destMaskAddr = 0;
    newAccessList->icmpParameters = NULL;
    newAccessList->igmpParameters = NULL;
    newAccessList->tcpOrUdpParameters = NULL;
    newAccessList->precedence = -1;
    newAccessList->tos = -1;
    newAccessList->established = FALSE;
    newAccessList->logType = ACCESS_LIST_NO_LOG;
    newAccessList->logOn = FALSE;
    newAccessList->packetCount = 0;
    newAccessList->timerSet = FALSE;
    newAccessList->continueNextLine = FALSE;
    memset(newAccessList->remark, 0, ACCESS_LIST_REMARK_STRING_SIZE);
    newAccessList->accessListNumber = 0;
    newAccessList->accessListName = NULL;
    newAccessList->isFreed = FALSE;
    newAccessList->next = NULL;
    newAccessList->isReflectDefined = FALSE;
    newAccessList->isDynamicallyCreated = FALSE;
    newAccessList->reflexParams = NULL;
    newAccessList->isProxy = TRUE;

    return newAccessList;
}

//--------------------------------------------------------------------------
// FUNCTION  AccessListInitTCPPortNameValue
// PURPOSE   Initialize the TCP port name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListInitTCPPortNameValue(NetworkDataIp* ip)
{
    ip->allNameValueLists->tcpPortNameAndNumber[0].portName =
        RtParseStringAllocAndCopy("bgp");
    ip->allNameValueLists->tcpPortNameAndNumber[0].portNumber = 179;

    ip->allNameValueLists->tcpPortNameAndNumber[1].portName =
        RtParseStringAllocAndCopy("chargen");
    ip->allNameValueLists->tcpPortNameAndNumber[1].portNumber = 19;

    ip->allNameValueLists->tcpPortNameAndNumber[2].portName =
        RtParseStringAllocAndCopy("cmd");
    ip->allNameValueLists->tcpPortNameAndNumber[2].portNumber = 514;

    ip->allNameValueLists->tcpPortNameAndNumber[3].portName =
        RtParseStringAllocAndCopy("daytime");
    ip->allNameValueLists->tcpPortNameAndNumber[3].portNumber = 13;

    ip->allNameValueLists->tcpPortNameAndNumber[4].portName =
        RtParseStringAllocAndCopy("discard");
    ip->allNameValueLists->tcpPortNameAndNumber[4].portNumber = 9;

    ip->allNameValueLists->tcpPortNameAndNumber[5].portName =
        RtParseStringAllocAndCopy("domain");
    ip->allNameValueLists->tcpPortNameAndNumber[5].portNumber = 53;

    ip->allNameValueLists->tcpPortNameAndNumber[6].portName =
        RtParseStringAllocAndCopy("echo");
    ip->allNameValueLists->tcpPortNameAndNumber[6].portNumber = 7;

    ip->allNameValueLists->tcpPortNameAndNumber[7].portName =
        RtParseStringAllocAndCopy("exec");
    ip->allNameValueLists->tcpPortNameAndNumber[7].portNumber = 512;

    ip->allNameValueLists->tcpPortNameAndNumber[8].portName =
        RtParseStringAllocAndCopy("finger");
    ip->allNameValueLists->tcpPortNameAndNumber[8].portNumber = 79;

    ip->allNameValueLists->tcpPortNameAndNumber[9].portName =
        RtParseStringAllocAndCopy("ftp");
    ip->allNameValueLists->tcpPortNameAndNumber[9].portNumber = 21;

    ip->allNameValueLists->tcpPortNameAndNumber[10].portName =
        RtParseStringAllocAndCopy("ftp-data");
    ip->allNameValueLists->tcpPortNameAndNumber[10].portNumber = 20;

    ip->allNameValueLists->tcpPortNameAndNumber[11].portName =
        RtParseStringAllocAndCopy("gopher");
    ip->allNameValueLists->tcpPortNameAndNumber[11].portNumber = 70;

    ip->allNameValueLists->tcpPortNameAndNumber[12].portName =
        RtParseStringAllocAndCopy("hostname");
    ip->allNameValueLists->tcpPortNameAndNumber[12].portNumber = 101;

    ip->allNameValueLists->tcpPortNameAndNumber[13].portName =
        RtParseStringAllocAndCopy("ident");
    ip->allNameValueLists->tcpPortNameAndNumber[13].portNumber = 113;

    ip->allNameValueLists->tcpPortNameAndNumber[14].portName =
        RtParseStringAllocAndCopy("irc");
    ip->allNameValueLists->tcpPortNameAndNumber[14].portNumber = 194;

    ip->allNameValueLists->tcpPortNameAndNumber[15].portName =
        RtParseStringAllocAndCopy("klogin");
    ip->allNameValueLists->tcpPortNameAndNumber[15].portNumber = 543;

    ip->allNameValueLists->tcpPortNameAndNumber[16].portName =
        RtParseStringAllocAndCopy("kshell");
    ip->allNameValueLists->tcpPortNameAndNumber[16].portNumber = 544;

    ip->allNameValueLists->tcpPortNameAndNumber[17].portName =
        RtParseStringAllocAndCopy("lpd");
    ip->allNameValueLists->tcpPortNameAndNumber[17].portNumber = 515;

    ip->allNameValueLists->tcpPortNameAndNumber[18].portName =
        RtParseStringAllocAndCopy("nntp");
    ip->allNameValueLists->tcpPortNameAndNumber[18].portNumber = 119;

    ip->allNameValueLists->tcpPortNameAndNumber[19].portName =
        RtParseStringAllocAndCopy("pop2");
    ip->allNameValueLists->tcpPortNameAndNumber[19].portNumber = 109;

    ip->allNameValueLists->tcpPortNameAndNumber[20].portName =
        RtParseStringAllocAndCopy("pop3");
    ip->allNameValueLists->tcpPortNameAndNumber[20].portNumber = 110;

    ip->allNameValueLists->tcpPortNameAndNumber[21].portName =
        RtParseStringAllocAndCopy("smtp");
    ip->allNameValueLists->tcpPortNameAndNumber[21].portNumber = 25;

    ip->allNameValueLists->tcpPortNameAndNumber[22].portName =
        RtParseStringAllocAndCopy("sunrpc");
    ip->allNameValueLists->tcpPortNameAndNumber[22].portNumber = 111;

    ip->allNameValueLists->tcpPortNameAndNumber[23].portName =
        RtParseStringAllocAndCopy("syslog");
    ip->allNameValueLists->tcpPortNameAndNumber[23].portNumber = 514;

    ip->allNameValueLists->tcpPortNameAndNumber[24].portName =
        RtParseStringAllocAndCopy("tacacs-ds");
    ip->allNameValueLists->tcpPortNameAndNumber[24].portNumber = 65;

    ip->allNameValueLists->tcpPortNameAndNumber[25].portName =
        RtParseStringAllocAndCopy("talk");
    ip->allNameValueLists->tcpPortNameAndNumber[25].portNumber = 517;

    ip->allNameValueLists->tcpPortNameAndNumber[26].portName =
        RtParseStringAllocAndCopy("telnet");
    ip->allNameValueLists->tcpPortNameAndNumber[26].portNumber = 23;

    ip->allNameValueLists->tcpPortNameAndNumber[27].portName =
        RtParseStringAllocAndCopy("time");
    ip->allNameValueLists->tcpPortNameAndNumber[27].portNumber = 37;

    ip->allNameValueLists->tcpPortNameAndNumber[28].portName =
        RtParseStringAllocAndCopy("uucp");
    ip->allNameValueLists->tcpPortNameAndNumber[28].portNumber = 540;

    ip->allNameValueLists->tcpPortNameAndNumber[29].portName =
        RtParseStringAllocAndCopy("whois");
    ip->allNameValueLists->tcpPortNameAndNumber[29].portNumber = 43;

    ip->allNameValueLists->tcpPortNameAndNumber[30].portName =
        RtParseStringAllocAndCopy("www");
    ip->allNameValueLists->tcpPortNameAndNumber[30].portNumber = 80;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListInitUDPPortNameValue
// PURPOSE   Initialize the UDP port name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListInitUDPPortNameValue(NetworkDataIp* ip)
{
    ip->allNameValueLists->udpPortNameAndNumber[0].portName
        = RtParseStringAllocAndCopy("biff");
    ip->allNameValueLists->udpPortNameAndNumber[0].portNumber = 512;

    ip->allNameValueLists->udpPortNameAndNumber[1].portName =
        RtParseStringAllocAndCopy("bootpc");
    ip->allNameValueLists->udpPortNameAndNumber[1].portNumber = 68;

    ip->allNameValueLists->udpPortNameAndNumber[2].portName =
        RtParseStringAllocAndCopy("bootps");
    ip->allNameValueLists->udpPortNameAndNumber[2].portNumber = 67;

    ip->allNameValueLists->udpPortNameAndNumber[3].portName =
        RtParseStringAllocAndCopy("discard");
    ip->allNameValueLists->udpPortNameAndNumber[3].portNumber = 9;

    ip->allNameValueLists->udpPortNameAndNumber[4].portName =
        RtParseStringAllocAndCopy("dns");
    ip->allNameValueLists->udpPortNameAndNumber[4].portNumber = 53;

    ip->allNameValueLists->udpPortNameAndNumber[5].portName =
        RtParseStringAllocAndCopy("dnsix");
    ip->allNameValueLists->udpPortNameAndNumber[5].portNumber = 195;

    ip->allNameValueLists->udpPortNameAndNumber[6].portName =
        RtParseStringAllocAndCopy("echo");
    ip->allNameValueLists->udpPortNameAndNumber[6].portNumber = 7;

    ip->allNameValueLists->udpPortNameAndNumber[7].portName =
        RtParseStringAllocAndCopy("mobile-ip");
    ip->allNameValueLists->udpPortNameAndNumber[7].portNumber = 434;

    ip->allNameValueLists->udpPortNameAndNumber[8].portName =
        RtParseStringAllocAndCopy("nameserver");
    ip->allNameValueLists->udpPortNameAndNumber[8].portNumber = 42;

    ip->allNameValueLists->udpPortNameAndNumber[9].portName =
        RtParseStringAllocAndCopy("netbios-dgm");
    ip->allNameValueLists->udpPortNameAndNumber[9].portNumber = 138;

    ip->allNameValueLists->udpPortNameAndNumber[10].portName =
        RtParseStringAllocAndCopy("netbios-ns");
    ip->allNameValueLists->udpPortNameAndNumber[10].portNumber = 137;

    ip->allNameValueLists->udpPortNameAndNumber[11].portName =
        RtParseStringAllocAndCopy("ntp");
    ip->allNameValueLists->udpPortNameAndNumber[11].portNumber = 123;

    ip->allNameValueLists->udpPortNameAndNumber[12].portName =
        RtParseStringAllocAndCopy("rip");
    ip->allNameValueLists->udpPortNameAndNumber[12].portNumber = 520;

    ip->allNameValueLists->udpPortNameAndNumber[13].portName =
        RtParseStringAllocAndCopy("snmp");
    ip->allNameValueLists->udpPortNameAndNumber[13].portNumber = 161;

    ip->allNameValueLists->udpPortNameAndNumber[14].portName =
        RtParseStringAllocAndCopy("snmptrap");
    ip->allNameValueLists->udpPortNameAndNumber[14].portNumber = 162;

    ip->allNameValueLists->udpPortNameAndNumber[15].portName =
        RtParseStringAllocAndCopy("sunrpc");
    ip->allNameValueLists->udpPortNameAndNumber[15].portNumber = 111;

    ip->allNameValueLists->udpPortNameAndNumber[16].portName =
        RtParseStringAllocAndCopy("syslog");
    ip->allNameValueLists->udpPortNameAndNumber[16].portNumber = 514;

    ip->allNameValueLists->udpPortNameAndNumber[17].portName =
        RtParseStringAllocAndCopy("tacacs-ds");
    ip->allNameValueLists->udpPortNameAndNumber[17].portNumber = 65;

    ip->allNameValueLists->udpPortNameAndNumber[18].portName =
        RtParseStringAllocAndCopy("talk");
    ip->allNameValueLists->udpPortNameAndNumber[18].portNumber = 517;

    ip->allNameValueLists->udpPortNameAndNumber[19].portName =
        RtParseStringAllocAndCopy("tftp");
    ip->allNameValueLists->udpPortNameAndNumber[19].portNumber = 69;

    ip->allNameValueLists->udpPortNameAndNumber[20].portName =
        RtParseStringAllocAndCopy("time");
    ip->allNameValueLists->udpPortNameAndNumber[20].portNumber = 69;

    ip->allNameValueLists->udpPortNameAndNumber[21].portName =
        RtParseStringAllocAndCopy("who");
    ip->allNameValueLists->udpPortNameAndNumber[21].portNumber = 513;

    ip->allNameValueLists->udpPortNameAndNumber[22].portName =
        RtParseStringAllocAndCopy("xdmcp");
    ip->allNameValueLists->udpPortNameAndNumber[22].portNumber = 177;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListInitICMPMessageNameValue
// PURPOSE   Initialize the ICMP name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
// COMMENTS  Several type and code were not found in the RFC792
//            for ICMP. So the default value for type is taken 256
//            and code is taken to be 0.
//--------------------------------------------------------------------------
static
void AccessListInitICMPMessageNameValue(NetworkDataIp* ip)
{
    ip->allNameValueLists->icmpMessageNameTypeAndCode[0].icmpName =
        RtParseStringAllocAndCopy("administratively-prohibited");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[0].icmpType = 3;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[0].icmpCode = 9;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[1].icmpName =
        RtParseStringAllocAndCopy("alternate-address");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[1].icmpType = 6;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[1].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[2].icmpName =
        RtParseStringAllocAndCopy("conversion-error");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[2].icmpType = 31;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[2].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[3].icmpName =
        RtParseStringAllocAndCopy("dod-host-prohibited");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[3].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[3].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[4].icmpName =
        RtParseStringAllocAndCopy("dod-net-prohibited");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[4].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[4].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[5].icmpName =
        RtParseStringAllocAndCopy("echo");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[5].icmpType = 8;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[5].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[6].icmpName =
        RtParseStringAllocAndCopy("echo-reply");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[6].icmpType = 0;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[6].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[7].icmpName =
        RtParseStringAllocAndCopy("general-parameter-problem");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[7].icmpType = 12;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[7].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[8].icmpName =
        RtParseStringAllocAndCopy("host-isolated");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[8].icmpType = 3;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[8].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[9].icmpName =
        RtParseStringAllocAndCopy("host-precedence-unreachable");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[9].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[9].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[10].icmpName =
        RtParseStringAllocAndCopy("host-redirect");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[10].icmpType = 5;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[10].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[11].icmpName =
        RtParseStringAllocAndCopy("host-tos-redirect");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[11].icmpType = 5;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[11].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[12].icmpName =
        RtParseStringAllocAndCopy("host-tos-unreachable");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[12].icmpType = 3;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[12].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[13].icmpName =
        RtParseStringAllocAndCopy("host-unknown");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[13].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[13].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[14].icmpName =
        RtParseStringAllocAndCopy("host-unreachable");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[14].icmpType = 3;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[14].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[15].icmpName =
        RtParseStringAllocAndCopy("information-reply");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[15].icmpType = 16;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[15].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[16].icmpName =
        RtParseStringAllocAndCopy("information-request");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[16].icmpType = 15;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[16].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[17].icmpName =
        RtParseStringAllocAndCopy("log");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[17].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[17].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[18].icmpName =
        RtParseStringAllocAndCopy("mask-reply");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[18].icmpType = 18;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[18].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[19].icmpName =
        RtParseStringAllocAndCopy("mask-request");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[19].icmpType = 17;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[19].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[20].icmpName =
        RtParseStringAllocAndCopy("mobile-redirect");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[20].icmpType = 32;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[20].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[21].icmpName =
        RtParseStringAllocAndCopy("net-redirect");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[21].icmpType = 5;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[21].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[22].icmpName =
        RtParseStringAllocAndCopy("net-tos-redirect");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[22].icmpType = 5;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[22].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[23].icmpName =
        RtParseStringAllocAndCopy("net-tos-unreachable");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[23].icmpType = 3;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[23].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[24].icmpName =
        RtParseStringAllocAndCopy("net-unreachable");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[24].icmpType = 3;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[24].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[25].icmpName =
        RtParseStringAllocAndCopy("network-unknown");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[25].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[25].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[26].icmpName =
        RtParseStringAllocAndCopy("no-room-for-option");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[26].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[26].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[27].icmpName =
        RtParseStringAllocAndCopy("option-missing");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[27].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[27].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[28].icmpName =
        RtParseStringAllocAndCopy("packet-too-big");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[28].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[28].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[29].icmpName =
        RtParseStringAllocAndCopy("parameter-problem");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[29].icmpType = 12;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[29].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[30].icmpName =
        RtParseStringAllocAndCopy("port-unreachable");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[30].icmpType = 3;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[30].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[31].icmpName =
        RtParseStringAllocAndCopy("precedence");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[31].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[31].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[32].icmpName =
        RtParseStringAllocAndCopy("precedence-unreachable");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[32].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[32].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[33].icmpName =
        RtParseStringAllocAndCopy("protocol-unreachable");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[33].icmpType = 3;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[33].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[34].icmpName =
        RtParseStringAllocAndCopy("reassembly-timeout");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[34].icmpType = 11;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[34].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[35].icmpName =
        RtParseStringAllocAndCopy("redirect");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[35].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[35].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[36].icmpName =
        RtParseStringAllocAndCopy("router-advertisement");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[36].icmpType = 9;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[36].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[37].icmpName =
        RtParseStringAllocAndCopy("router-solicitation");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[37].icmpType = 10;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[37].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[38].icmpName =
        RtParseStringAllocAndCopy("source-quench");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[38].icmpType = 4;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[38].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[39].icmpName =
        RtParseStringAllocAndCopy("source-route-failed");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[39].icmpType = 3;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[39].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[40].icmpName =
        RtParseStringAllocAndCopy("time-exceeded");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[40].icmpType = 11;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[40].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[41].icmpName =
        RtParseStringAllocAndCopy("timestamp-reply");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[41].icmpType = 14;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[41].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[42].icmpName =
        RtParseStringAllocAndCopy("timestamp-request");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[42].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[42].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[43].icmpName =
        RtParseStringAllocAndCopy("tos");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[43].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[43].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[44].icmpName =
        RtParseStringAllocAndCopy("traceroute");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[44].icmpType = 30;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[44].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[45].icmpName =
        RtParseStringAllocAndCopy("ttl-exceeded");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[45].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[45].icmpCode = 0;

    ip->allNameValueLists->icmpMessageNameTypeAndCode[46].icmpName =
        RtParseStringAllocAndCopy("unreachable");
    ip->allNameValueLists->icmpMessageNameTypeAndCode[46].icmpType = 256;
    ip->allNameValueLists->icmpMessageNameTypeAndCode[46].icmpCode = 0;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListInitIGMPMessageNameValue
// PURPOSE   Initialize the IGMP name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
// COMMENTS  Several type and code were not found for IGMP. So
//           temporarily we are assigning the ones used by Qualnet
//--------------------------------------------------------------------------
static
void AccessListInitIGMPMessageNameValue(NetworkDataIp* ip)
{
    ip->allNameValueLists->igmpMessageNameType[0].igmpName =
        RtParseStringAllocAndCopy("IGMP_QUERY_MSG");
    ip->allNameValueLists->igmpMessageNameType[0].igmpType = 17;

    ip->allNameValueLists->igmpMessageNameType[1].igmpName =
        RtParseStringAllocAndCopy("IGMP_DVMRP");
    ip->allNameValueLists->igmpMessageNameType[1].igmpType = 19;

    ip->allNameValueLists->igmpMessageNameType[2].igmpName =
        RtParseStringAllocAndCopy("IGMP_MEMBERSHIP_REPORT_MSG");
    ip->allNameValueLists->igmpMessageNameType[2].igmpType = 22;

    ip->allNameValueLists->igmpMessageNameType[3].igmpName =
        RtParseStringAllocAndCopy("IGMP_LEAVE_GROUP_MSG");
    ip->allNameValueLists->igmpMessageNameType[3].igmpType = 23;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListSearchPrecedence
// PURPOSE   Initialize the various name value list
// ARGUMENTS strToSearch, string to match
//           ip, ip data structure
// RETURN    Value of precedence search
//--------------------------------------------------------------------------
static
int AccessListSearchPrecedence(char* strToSearch,
                               NetworkDataIp* ip)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_PRECEDENCE_MAX; iterate++)
    {
        if (RtParseStringNoCaseCompare(strToSearch,
            ip->allNameValueLists->precedenceList
            [iterate].precedenceName) == 0)
        {
            // if the name matches
            return ip->allNameValueLists->precedenceList
                [iterate].precedenceValue;
        }
    }
    return -1;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListIsPrecedenceValue
// PURPOSE   Search UDP port name value list
// ARGUMENTS numberToCheck, number to match
//           ip, ip data structure
// RETURN    BOOL value result of precedence match
//--------------------------------------------------------------------------
static
BOOL AccessListIsPrecedenceValue(int numberToCheck,
                                 NetworkDataIp* ip)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_PRECEDENCE_MAX; iterate++)
    {
        if (numberToCheck == ip->allNameValueLists->precedenceList
            [iterate].precedenceValue)
        {
            // if the number matches
            return TRUE;
        }
    }
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListSearchTCPPortNameValue
// PURPOSE   Initialize the prcedence name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
// COMMENT   The coresponding precedence numbers are matched as declared in
//            ip.h pf Qualnet. The CISCO document has just mentioned it in
//            alphabetical order.
//--------------------------------------------------------------------------
static
void AccessListInitPrecedence(NetworkDataIp* ip)
{
    ip->allNameValueLists->precedenceList[0].precedenceName =
        RtParseStringAllocAndCopy("critical");
    ip->allNameValueLists->precedenceList[0].precedenceValue = 5;

    ip->allNameValueLists->precedenceList[1].precedenceName =
        RtParseStringAllocAndCopy("flash");
    ip->allNameValueLists->precedenceList[1].precedenceValue = 3;

    ip->allNameValueLists->precedenceList[2].precedenceName =
        RtParseStringAllocAndCopy("flash-override");
    ip->allNameValueLists->precedenceList[2].precedenceValue = 4;

    ip->allNameValueLists->precedenceList[3].precedenceName =
        RtParseStringAllocAndCopy("immediate");
    ip->allNameValueLists->precedenceList[3].precedenceValue = 2;

    ip->allNameValueLists->precedenceList[4].precedenceName =
        RtParseStringAllocAndCopy("internet");
    ip->allNameValueLists->precedenceList[4].precedenceValue = 6;

    ip->allNameValueLists->precedenceList[5].precedenceName =
        RtParseStringAllocAndCopy("network");
    ip->allNameValueLists->precedenceList[5].precedenceValue = 7;

    ip->allNameValueLists->precedenceList[6].precedenceName =
        RtParseStringAllocAndCopy("priority");
    ip->allNameValueLists->precedenceList[6].precedenceValue = 1;

    ip->allNameValueLists->precedenceList[7].precedenceName =
        RtParseStringAllocAndCopy("routine");
    ip->allNameValueLists->precedenceList[7].precedenceValue = 0;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListSearchTOS
// PURPOSE   Initialize the various name value list
// ARGUMENTS strToSearch, string to match
//           ip, ip data structure
// RETURN    Value of corresponding TOS or not found value
//--------------------------------------------------------------------------
static
int AccessListSearchTOS(char* strToSearch,
                        NetworkDataIp* ip)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_TOS_MAX; iterate++)
    {
        if (RtParseStringNoCaseCompare(strToSearch, ip->
            allNameValueLists->tosList[iterate].tosName) == 0)
        {
            // if the name matches
            return ip->allNameValueLists->tosList
                [iterate].tosValue;
        }
    }
    return -1;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListIsTOSValue
// PURPOSE   Search TOS name value list
// ARGUMENTS numberToCheck, number to match
//           ip, ip data structure
// RETURN    BOOL result of TOS match
//--------------------------------------------------------------------------
static
BOOL AccessListIsTOSValue(int numberToCheck, NetworkDataIp* ip)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_TOS_MAX; iterate++)
    {
        if (numberToCheck == ip->allNameValueLists->tosList
            [iterate].tosValue)
        {
            // if the number matches
            return TRUE;
        }
    }
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListInitTOS
// PURPOSE   Initialize the various name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListInitTOS(NetworkDataIp* ip)
{
    ip->allNameValueLists->tosList[0].tosName = RtParseStringAllocAndCopy(
        "max-reliability");
    ip->allNameValueLists->tosList[0].tosValue = 2;

    ip->allNameValueLists->tosList[1].tosName = RtParseStringAllocAndCopy(
        "max-throughput");
    ip->allNameValueLists->tosList[1].tosValue = 4;

    ip->allNameValueLists->tosList[2].tosName = RtParseStringAllocAndCopy(
        "min-delay");
    ip->allNameValueLists->tosList[2].tosValue = 8;

    ip->allNameValueLists->tosList[3].tosName = RtParseStringAllocAndCopy(
        "min-monetary-cost");
    ip->allNameValueLists->tosList[3].tosValue = 1;

    ip->allNameValueLists->tosList[4].tosName = RtParseStringAllocAndCopy(
        "normal");
    ip->allNameValueLists->tosList[4].tosValue = 0;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListInitNameValue
// PURPOSE   Initialize the various name value lists
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListInitNameValue(NetworkDataIp* ip)
{
    ip->allNameValueLists = (AccessListParameterNameValue*)
        RtParseMallocAndSet(sizeof(AccessListParameterNameValue));

    //initialize name value
    AccessListInitTCPPortNameValue(ip);
    AccessListInitUDPPortNameValue(ip);
    AccessListInitICMPMessageNameValue(ip);
    AccessListInitIGMPMessageNameValue(ip);
    AccessListInitPrecedence(ip);
    AccessListInitTOS(ip);
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListFreeTCPPortNameValue
// PURPOSE   Freeing the tcp name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListFreeTCPPortNameValue(NetworkDataIp* ip)
{
    int i = 0;

    for (i = 0; i < ACCESS_LIST_TCP_MAX_PORT; i++)
    {
        MEM_free(ip->allNameValueLists->tcpPortNameAndNumber[i].portName);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListFreeUDPPortNameValue
// PURPOSE   Freeing the tcp name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListFreeUDPPortNameValue(NetworkDataIp* ip)
{
    int i = 0;

    for (i = 0; i < ACCESS_LIST_UDP_MAX_PORT; i++)
    {
        MEM_free(ip->allNameValueLists->udpPortNameAndNumber[i].portName);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListFreeICMPMessageNameValue
// PURPOSE   Freeing the icmp name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListFreeICMPMessageNameValue(NetworkDataIp* ip)
{
    int i = 0;

    for (i = 0; i < ACCESS_LIST_ICMP_MAX_MESSAGE; i++)
    {
        MEM_free(ip->allNameValueLists->icmpMessageNameTypeAndCode[i].
            icmpName);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListFreeIGMPMessageNameValue
// PURPOSE   Freeing the icmp name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListFreeIGMPMessageNameValue(NetworkDataIp* ip)
{
    int i = 0;

    for (i = 0; i < ACCESS_LIST_IGMP_MAX_MESSAGE; i++)
    {
        MEM_free(ip->allNameValueLists->igmpMessageNameType[i].igmpName);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListFreePrecedence
// PURPOSE   Freeing the precedence name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListFreePrecedence(NetworkDataIp* ip)
{
    int i = 0;

    for (i = 0; i < ACCESS_LIST_PRECEDENCE_MAX; i++)
    {
        MEM_free(ip->allNameValueLists->precedenceList[i].precedenceName);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListFreeTOS
// PURPOSE   Freeing the tos name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListFreeTOS(NetworkDataIp* ip)
{
    int i = 0;

    for (i = 0; i < ACCESS_LIST_TOS_MAX; i++)
    {
        MEM_free(ip->allNameValueLists->tosList[i].tosName);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListFreeNameValue
// PURPOSE   Freeing the various name value list
// ARGUMENTS ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListFreeNameValue(NetworkDataIp* ip)
{
    AccessListFreeTCPPortNameValue(ip);
    AccessListFreeUDPPortNameValue(ip);
    AccessListFreeICMPMessageNameValue(ip);
    AccessListFreeIGMPMessageNameValue(ip);
    AccessListFreePrecedence(ip);
    AccessListFreeTOS(ip);

    MEM_free(ip->allNameValueLists);
    ip->allNameValueLists = NULL;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListSearchTCPPortNameValue
// PURPOSE   Initialize the various name value list
// ARGUMENTS strToSearch, string to match
//           ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
int AccessListSearchTCPPortNameValue(char* strToSearch,
                                     NetworkDataIp* ip)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_TCP_MAX_PORT; iterate++)
    {
        if (RtParseStringNoCaseCompare(strToSearch, ip->
            allNameValueLists->tcpPortNameAndNumber[iterate].portName) == 0)
        {
            // if the name matches
            return ip->allNameValueLists->tcpPortNameAndNumber[iterate].
                portNumber;
        }
    }
    return -1;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListSearchUDPPortNameValue
// PURPOSE   Search UDP port name value list
// ARGUMENTS strToSearch, string to match
//           ip, ip data structure
// RETURN    None
//--------------------------------------------------------------------------
static
int AccessListSearchUDPPortNameValue(char* strToSearch,
                                     NetworkDataIp* ip)
{
    int iterate = 0;

    for (iterate = 0; iterate < ACCESS_LIST_UDP_MAX_PORT; iterate++)
    {
        if (RtParseStringNoCaseCompare(strToSearch, ip->
            allNameValueLists->udpPortNameAndNumber[iterate].portName) == 0)
        {
            // if the name matches
            return ip->allNameValueLists->udpPortNameAndNumber[iterate].
                portNumber;
        }
    }
    return -1;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListSearchICMPNameValue
// PURPOSE   Initialize the various name value list
// ARGUMENTS strToSearch, string to match
//           ip, ip data structure
//           returnType, what to return, type or code
// RETURN    None
//--------------------------------------------------------------------------
static
int AccessListSearchICMPNameValue(char* strToSearch,
                                  NetworkDataIp* ip,
                                  int* returnType,
                                  int* code)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_ICMP_MAX_MESSAGE; iterate++)
    {
        if (RtParseStringNoCaseCompare(strToSearch,
            ip->allNameValueLists->icmpMessageNameTypeAndCode[iterate].
            icmpName) == 0)
        {
            *returnType = ip->allNameValueLists->icmpMessageNameTypeAndCode
                [iterate].icmpType;
            *code = ip->allNameValueLists->icmpMessageNameTypeAndCode
                [iterate].icmpCode;
            return 1;
        }
    }
    return -1;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListIsICMPTypeOrCode
// PURPOSE   Search UDP port name value list
// ARGUMENTS numberToCheck, number to match
//           ip, ip data structure
//           isType, what to match, type or code
// RETURN    BOOL result for ICMP match
//--------------------------------------------------------------------------
static
BOOL AccessListIsICMPTypeOrCode(int numberToCheck,
                                NetworkDataIp* ip,
                                BOOL isType)
{
    int iterate = 0;

    if (isType == TRUE)
    {
        for (iterate = 0; iterate <= ACCESS_LIST_ICMP_MAX_MESSAGE; iterate++)
        {
            if (ip->allNameValueLists->icmpMessageNameTypeAndCode
                [iterate].icmpType == numberToCheck)
            {
                // if the number exits
                return TRUE;
            }
        }
    }
    else
    {
        for (iterate = 0; iterate <= ACCESS_LIST_ICMP_MAX_MESSAGE; iterate++)
        {
            if (ip->allNameValueLists->icmpMessageNameTypeAndCode
                [iterate].icmpCode == numberToCheck)
            {
                // if the number exits
                return TRUE;
            }
        }
    }
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListSearchIGMPNameValue
// PURPOSE   Initialize the various name value list
// ARGUMENTS strToSearch, string to match
//           ip, ip data structure
//           type, what to return, type or code
// RETURN    int result of match
//--------------------------------------------------------------------------
static
int AccessListSearchIGMPNameValue(char* strToSearch,
                                  NetworkDataIp* ip,
                                  int* type)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_IGMP_MAX_MESSAGE; iterate++)
    {
        if (RtParseStringNoCaseCompare(strToSearch, ip->
            allNameValueLists->igmpMessageNameType[iterate].igmpName) == 0)
        {
            *type = ip->allNameValueLists->igmpMessageNameType
                [iterate].igmpType;
            return 1;
        }
    }
    return -1;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListIsIGMPType
// PURPOSE   Search IGMP name value list
// ARGUMENTS numberToCheck, number to match
//           ip, ip data structure
//           isType, what to match, type or code
// RETURN    None
//--------------------------------------------------------------------------
static
BOOL AccessListIsIGMPType(int numberToCheck,
                          NetworkDataIp* ip)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_IGMP_MAX_MESSAGE; iterate++)
    {
        if (ip->allNameValueLists->igmpMessageNameType
            [iterate].igmpType == numberToCheck)
        {
            // if the number exits
            return TRUE;
        }
    }
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListIsUDPPortNumber
// PURPOSE   Search UDP port name value list
// ARGUMENTS numberToCheck, number to match
//           ip, ip data structure
// RETURN    BOOL result of UDP match
//--------------------------------------------------------------------------
static
BOOL AccessListIsUDPPortNumber(int numberToCheck, NetworkDataIp* ip)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_UDP_MAX_PORT; iterate++)
    {
        if (ip->allNameValueLists->udpPortNameAndNumber
            [iterate].portNumber == numberToCheck)
        {
            // if the number exits
            return TRUE;
        }
    }
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListIsTCPPortNumber
// PURPOSE   Search UDP port name value list
// ARGUMENTS numberToCheck, number to match
//           ip, ip data structure
// RETURN    BOOL result of TCP port match
//--------------------------------------------------------------------------
static
BOOL AccessListIsTCPPortNumber(int numberToCheck,
                               NetworkDataIp* ip)
{
    int iterate = 0;

    for (iterate = 0; iterate <= ACCESS_LIST_TCP_MAX_PORT; iterate++)
    {
        if (ip->allNameValueLists->tcpPortNameAndNumber
            [iterate].portNumber == numberToCheck)
        {
            // if the number exits
            return TRUE;
        }
    }
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListParsePrecedence
// PURPOSE   Parse the prcedence string
// ARGUMENTS node, particular node
//           originalString, the criteria string
//           token, the to parse token
//           newAccessList, current Access List pointer
//           ip, ip data pointer
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListParsePrecedence(Node* node,
                               const char* originalString,
                               char* token,
                               AccessList* newAccessList,
                               NetworkDataIp* ip)
{
    // if precedence is there

    if (isalpha((int)*token))
    {
        int returnVal = -1;
        returnVal = AccessListSearchPrecedence(token, ip);
        if (returnVal == -1)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "Node: %u\nWrong syntax in precedence name"
                "\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
        else
        {
            newAccessList->precedence = (unsigned char)returnVal;
        }
    }
    else if (isdigit((int)*token))
    {
        // precedence number is given
        if (AccessListIsPrecedenceValue(atoi(token), ip) == TRUE)
        {
            newAccessList->precedence = (unsigned char)atoi(token);
        }
        else
        {
            // this number is not permissible
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "Node: %u\nWrong precedence number in "
                "precedence parameter.\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // this is junk
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\nWrong syntax in precedence parameter"
            "\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListParseTOS
// PURPOSE   Check operator port
// ARGUMENTS originalString, the criteria string
//           token, the to parse token
//           newAccessList, current Access List pointer
//           ip, ip data pointer
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListParseTOS(Node* node,
                        const char* originalString,
                        char* token,
                        AccessList* newAccessList,
                        NetworkDataIp* ip)
{
    // if tos is there

    if (isalpha((int)*token))
    {
        int returnVal = 0;
        returnVal = AccessListSearchTOS(token, ip);
        if (returnVal == -1)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "Node: %u\nWrong syntax in precedence name"
                "'%s'\n",node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
        else
        {
            newAccessList->tos = (unsigned char) returnVal;
        }
    }
    else if (isdigit((int)*token))
    {
        // precedence number is given
        if (AccessListIsTOSValue(atoi(token), ip) == TRUE)
        {
            newAccessList->tos = (unsigned char) atoi(token);
        }
        else
        {
            // this number is not permissible
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode: %u\nWrong precedence number in "
                "precedence parameter.\n'%s'\n",node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // this is junk
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode: %u\nWrong syntax in precedence "
            "parameter.\n'%s'\n",node->nodeId, originalString);
        ERROR_ReportError(errString);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListParseLog
// PURPOSE   Check operator port
// ARGUMENTS originalString, the criteria string
//           token, the to parse token
//           newAccessList, current Access List pointer
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListParseLog(Node* node,
                        const char* originalString,
                        char* token,
                        AccessList* newAccessList)
{
    if (RtParseStringNoCaseCompare(token, "log") == 0)
    {
        newAccessList->logType = ACCESS_LIST_LOG;
        newAccessList->logOn = FALSE;
    }
    else if (RtParseStringNoCaseCompare(token, "log-input") == 0)
    {
        newAccessList->logType = ACCESS_LIST_LOG_INPUT;
        newAccessList->logOn = FALSE;
    }
    else
    {
        // this is junk
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode: %u\nWrong syntax in log parameter."
            "\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    newAccessList->packetCount = 0;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListParseAddress
// PURPOSE   Check operator port
// ARGUMENTS node, node being referenced
//           originalString, the criteria string
//           criteriaString, string to be parsed
//           newAccessList, current Access List pointer
//           isSource, is source(TRUE) or destination
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListParseAddress(Node* node,
                            const char* originalString,
                            char** criteriaString,
                            AccessList* newAccessList,
                            BOOL isSource)
{
    char token1[MAX_STRING_LENGTH];
    NodeAddress address;
    NodeAddress mask;
    BOOL isNodeId;
    int currentPosition = 0;

    sscanf(*criteriaString, "%s", token1);

    if (RtParseStringNoCaseCompare(token1, "ANY") == 0)
    {
        if (isSource == TRUE)
        {
            newAccessList->srcAddr = ~(ANY_DEST);
            newAccessList->srcMaskAddr = ANY_DEST;
        }
        else
        {
            newAccessList->destAddr = ~(ANY_DEST);
            newAccessList->destMaskAddr = ANY_DEST;
        }
    }
    else if (RtParseStringNoCaseCompare(token1, "HOST") == 0)
    {
        int dotCount = 0;

        // update the string
        currentPosition = RtParseGetPositionInString(
            *criteriaString, token1);
        *criteriaString = *criteriaString + currentPosition;

        if (**criteriaString == '\0')
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for standard "
                "access list in router config file\n'%s'\n",node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        sscanf(*criteriaString, "%s", token1);

        // we do the proper address check here than let it for fileio.c
        dotCount = RtParseCountDotSeparator(token1);
        if (dotCount != 3)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for standard "
                "access list in router config file\n'%s'\n",node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        IO_ParseNodeIdOrHostAddress(token1, &address, &isNodeId);
        if (isNodeId)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for node address "
                "in router config file\n'%s'\n",node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
        else
        {
            if (isSource == TRUE)
            {
                newAccessList->srcAddr = address;
                newAccessList->srcMaskAddr = ~(ANY_DEST);
            }
            else
            {
                newAccessList->destAddr = address;
                newAccessList->destMaskAddr = ~(ANY_DEST);
            }
        }
    }
    else
    {
        int dotCount = RtParseCountDotSeparator(token1);
        if (dotCount != 3)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for standard "
                "access list in router config file\n'%s'\n",node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        IO_ParseNodeIdOrHostAddress(token1, &address, &isNodeId);
        if (isNodeId)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for node "
                "address in router config file\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        // move to the mask
        currentPosition = RtParseGetPositionInString(*criteriaString,
            token1);
        *criteriaString = *criteriaString + currentPosition;
        if (**criteriaString == '\0')
        {
            if (isSource == TRUE)
            {
                newAccessList->srcAddr = address;
                newAccessList->srcMaskAddr = ~(ANY_DEST);
            }
            else
            {
                newAccessList->destAddr = address;
                newAccessList->destMaskAddr = ~(ANY_DEST);
            }
            return;
        }

        sscanf(*criteriaString, "%s", token1);

        // else log or mask can be the only option
        if (RtParseStringNoCaseCompare("log", token1) == 0)
        {
            if (isSource == TRUE)
            {
                newAccessList->srcAddr = address;
                newAccessList->srcMaskAddr = ~(ANY_DEST);
            }
            else
            {
                newAccessList->destAddr = address;
                newAccessList->destMaskAddr = ~(ANY_DEST);
            }
            return;
        }

        dotCount = 0;
        dotCount = RtParseCountDotSeparator(token1);
        if (dotCount != 3)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for standard "
                "access list in router config file\n'%s'\n",node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        IO_ParseNodeIdOrHostAddress(token1, &mask, &isNodeId);
        if (isNodeId)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for node "
                "address in router config file\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        // masking rule in CISCO access lists
        if (isSource == TRUE)
        {
            newAccessList->srcAddr = address;
            newAccessList->srcMaskAddr = mask;
        }
        else
        {
            newAccessList->destAddr = address;
            newAccessList->destMaskAddr = mask;
        }
    }

    // lets update the string and return
    currentPosition = RtParseGetPositionInString(
        *criteriaString, token1);
    *criteriaString = *criteriaString + currentPosition;
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListCheckAndParseOperatorPort
// PURPOSE   Check operator port
// ARGUMENTS node, corresponding node
//           originalString, the criteria string
//           ip, IP data structure
//           operatorString, type of operator
//           port, port number or name
//           protocol, typpe fo protocol
//           portNumber, port number to return
//           operatorType, operator type returned
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListCheckAndParseOperatorPort(
     Node* node,
     const char* originalString,
     NetworkDataIp* ip,
     char* operatorString,
     char* portString,
     unsigned char protocol,
     int* portNumber,
     AccessListOperatorType* operatorType,
     BOOL* foundPort)
{
    int portVal = -1;
    // checking operators
    if (RtParseStringNoCaseCompare(operatorString, "LT") == 0)
    {
        // < operator
        *operatorType = ACCESS_LIST_LESS_THAN;
    }
    else if (RtParseStringNoCaseCompare(operatorString, "GT") == 0)
    {
        // > operator
        *operatorType = ACCESS_LIST_GREATER_THAN;
    }
    else if (RtParseStringNoCaseCompare(operatorString, "EQ") == 0)
    {
        // == operator
        *operatorType = ACCESS_LIST_EQUALITY;
    }
    else if (RtParseStringNoCaseCompare(operatorString, "NEQ") == 0)
    {
        // != operator
        *operatorType = ACCESS_LIST_NOT_EQUAL;
    }
    else if (RtParseStringNoCaseCompare(operatorString,
        "ACCESS_LIST_RANGE") == 0)
    {
        // range operator
        *operatorType = ACCESS_LIST_RANGE;
    }
    else
    {
        // doesn't have the operator token, so lets return
        *foundPort = FALSE;
        return;
    }

    // we have the operator token, we must have the ports
    if (isalpha((int)*portString))
    {
        // port name is given as input in config file
        if ((int)protocol == IPPROTO_TCP)
        {
            portVal = AccessListSearchTCPPortNameValue(portString, ip);
        }
        else if ((int)protocol == IPPROTO_UDP)
        {
            portVal = AccessListSearchUDPPortNameValue(portString, ip);
        }
        else
        {
            // TCP and UDP can only have port parameters
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nPort parameters can be used "
                "only with TCP or UDP. \n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        if (portVal == -1)
        {
            // Port names used in access list can be only one of those allowed
            //  as given in CISCO doc's.
            // Ref: http://www.cisco.com/univercd/cc/td/doc/product/atm/c8540/
            //  12_0/13_19/cmd_ref/a.htm#xtocid173071
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nPort name not allowed in "
                "access list only with TCP or UDP. \n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
        else
        {
            *portNumber = portVal;
            *foundPort = TRUE;
        }
    }
    else if (isdigit((int)*portString))
    {
        BOOL isPort = FALSE;
        // port number is given as input in config file
        if ((int)protocol == IPPROTO_TCP)
        {
            isPort = AccessListIsTCPPortNumber(atoi(portString), ip);
        }
        else if ((int)protocol == IPPROTO_UDP)
        {
            isPort = AccessListIsUDPPortNumber(atoi(portString), ip);
        }
        else
        {
            // TCP and UDP can only have port parameters
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nPort parameters can be used "
                "only with TCP or UDP. \n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        if (isPort == FALSE)
        {
            // Port names used in access list can be only one of those
            // allowed as given in CISCO doc's.
            // Ref: http://www.cisco.com/univercd/cc/td/doc/product/atm
            // /c8540/12_0/13_19/cmd_ref/a.htm#xtocid173071
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong port number in config "
                "file.\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
            *foundPort = FALSE;
        }
        else
        {
            *portNumber = atoi(portString);
            *foundPort = TRUE;
        }
    }
    else
    {
        // port parameter can be either name or number
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nPort parameters can be either"
            "name or number.\n'%s'", node->nodeId, originalString);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListParseTCPorUDPCriteria
// PURPOSE   Parse the name access list
// ARGUMENTS node, The node initializing
//           originalString, the original criteria string
//           ip, ip data pointer
//           criteriaString, the remaining parse string
//           newAccessList, Pointer to Access List
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListParseTCPorUDPCriteria(Node* node,
                                     const char* originalString,
                                     NetworkDataIp* ip,
                                     char** criteriaString,
                                     AccessList* newAccessList)
{
    char token1[MAX_STRING_LENGTH];
    char token2[MAX_STRING_LENGTH];
    int cutPosition = 0;
    BOOL foundPort = FALSE;

    AccessListTCPorUDPParams* subParam;
    AccessListOperatorType portOperator;
    int portNumber = 0;

    // parse the source address
    AccessListParseAddress(node,
                           originalString,
                           criteriaString,
                           newAccessList,
                           TRUE);

    if (**criteriaString == '\0')
    {
        // string has terminated abnoramlly, so an error,
        // missing destination address
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nMissing Destination address in"
            " Extended Access list.\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // optional parameter operator
    sscanf(*criteriaString, "%s %s", token1, token2);

    // if there are any source port operators then parse
    AccessListCheckAndParseOperatorPort(node,
                                        originalString,
                                        ip,
                                        token1,
                                        token2,
                                        newAccessList->protocol,
                                        &portNumber,
                                        &portOperator,
                                        &foundPort);

    if (foundPort == TRUE)
    {
        int currentPosition = 0;

        // allocating memory for TCP/UDP params
        subParam = (AccessListTCPorUDPParams*) RtParseMallocAndSet(sizeof(
            AccessListTCPorUDPParams));

        subParam->srcOperator = portOperator;
        subParam->srcPortNumber = portNumber;
        newAccessList->tcpOrUdpParameters = subParam;
        cutPosition = 2;

        currentPosition = RtParseGetPositionInString(
            *criteriaString, token2);
        *criteriaString = *criteriaString + currentPosition;

        if (**criteriaString == '\0')
        {
            // string has terminated abnoramlly, so an error,
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax format for "
                "Access list. Missing destination address.\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
    }

    // parse dest address and dest ports
    AccessListParseAddress(node,
                           originalString,
                           criteriaString,
                           newAccessList,
                           FALSE);

    if (**criteriaString == '\0')
    {
        // string has terminated, so return
        return;
    }

    // parse dest ports if any
    sscanf(*criteriaString, "%s %s", token1, token2);

    // if there are any port operators then parse
    AccessListCheckAndParseOperatorPort(node,
                                        originalString,
                                        ip,
                                        token1,
                                        token2,
                                        newAccessList->protocol,
                                        &portNumber,
                                        &portOperator,
                                        &foundPort);

    if (foundPort == TRUE)
    {
        if (newAccessList->tcpOrUdpParameters == NULL)
        {
            // allocating memory for TCP/UDP params
            newAccessList->tcpOrUdpParameters =
                (AccessListTCPorUDPParams*) RtParseMallocAndSet(sizeof(
                AccessListTCPorUDPParams));
            newAccessList->tcpOrUdpParameters->destOperator =
                portOperator;
            newAccessList->tcpOrUdpParameters->destPortNumber =
                portNumber;
        }
        else
        {
            newAccessList->tcpOrUdpParameters->destOperator =
                portOperator;
            newAccessList->tcpOrUdpParameters->destPortNumber =
                portNumber;
        }
        cutPosition = 2;
    }
    else
    {
        cutPosition = 0;
    }

    if (cutPosition == 2)
    {
        int currentPosition = 0;
        currentPosition = RtParseGetPositionInString(
            *criteriaString, token2);

        // update to the next token count
        *criteriaString = *criteriaString + currentPosition;
    }

    if (**criteriaString == '\0')
    {
        return;
    }

    // tcp has got another extra field "established". The CISCO doc used
    // this param along with dynamic access lists. But we still implement
    // this param. Ref: http://www.cisco.com/univercd/cc/td/doc/product/
    //  software/ios122/122cgcr/fipras_r/1rfip1.htm
    if (newAccessList->protocol == IPPROTO_TCP)
    {
        sscanf(*criteriaString, "%s", token1);
        if (RtParseStringNoCaseCompare(token1, "ESTABLISHED") == 0)
        {
            int currentPosition = 0;

            newAccessList->established = TRUE;
            currentPosition = RtParseGetPositionInString(
                *criteriaString, token1);

            // update to the next token
            *criteriaString = *criteriaString + currentPosition;
        }
    }

    // rest three optional tokens are same for all protocols,
    // so return to AccessListParseCriteria
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListParseICMPCriteria
// PURPOSE   Parse the name access list
// ARGUMENTS node, The node initializing
//           originalString, the original criteria
//           ip, ip data pointer
//           criteriaString, the remaining parse string
//           newAccessList, pointer to Access List
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListParseICMPCriteria(Node* node,
                                 const char* originalString,
                                 NetworkDataIp* ip,
                                 char** criteriaString,
                                 AccessList* newAccessList)
{
    char token1[MAX_STRING_LENGTH];
    char token2[MAX_STRING_LENGTH];
    int cutTokenCount = 0;
    int currentPosition = 0;

    // parse the source Address
    AccessListParseAddress(node,
                           originalString,
                           criteriaString,
                           newAccessList,
                           TRUE);

    if (**criteriaString == '\0')
    {
        // string has terminated abnoramlly, so an error,
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax format for "
            "Access list. Missing destination address.\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // parse destination address
    AccessListParseAddress(node,
                           originalString,
                           criteriaString,
                           newAccessList,
                           FALSE);

    // if ths string has terminated
    if (**criteriaString == '\0')
    {
        return;
    }

    // optional icmp parameters
    // [ type[code] | message ]
    // ex: 3 1
    sscanf(*criteriaString, "%s %s", token1, token2);
    if (isalpha((int)*token1))
    {
        // is a string so is it ICMP message name
        int type = -1;
        int code = -1;

        if (AccessListSearchICMPNameValue(token1,
                                          ip,
                                          &type,
                                          &code) == 1)
        {
            if (newAccessList->icmpParameters == NULL)
            {
                newAccessList->icmpParameters
                    = (AccessListICMPParams *)
                      RtParseMallocAndSet(sizeof(AccessListICMPParams));
                newAccessList->icmpParameters->icmpType = type;
                newAccessList->icmpParameters->icmpCode = code;
            }
            cutTokenCount = 1;
        }
        else
        {
            cutTokenCount = 0;
        }
    }
    else if (isdigit((int)*token1))
    {
        // is a number, match the type
        if (AccessListIsICMPTypeOrCode(atoi(token1),
                                       ip,
                                       TRUE) ==  TRUE)
        {
            // now check the code
            if (isdigit((int)*token2))
            {
                if (AccessListIsICMPTypeOrCode(atoi(token2), ip, FALSE)
                    == TRUE)
                {
                    // found code, intitialize the type and code
                    if (newAccessList->icmpParameters == NULL)
                    {
                        newAccessList->icmpParameters
                            = (AccessListICMPParams *)
                              RtParseMallocAndSet(sizeof(
                                  AccessListICMPParams));
                        newAccessList->icmpParameters->icmpType = atoi
                            (token1);
                        newAccessList->icmpParameters->icmpCode = atoi
                            (token2);
                    }
                    else
                    {
                        char errString[MAX_STRING_LENGTH];
                        sprintf(errString, "\nNode :%u\nError for ICMP"
                            " field initialization\n'%s'\n",
                            node->nodeId, originalString);
                        ERROR_ReportError(errString);
                    }
                    cutTokenCount = 2;
                }
                else
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "\nNode :%u\nAccess list ICMP code "
                        "in router config is not valid\n'%s'\n",
                        node->nodeId, originalString);
                    ERROR_ReportError(errString);
                }
            }
            else
            {
                // no codes so just initialize the type
                if (newAccessList->icmpParameters == NULL)
                {
                    newAccessList->icmpParameters
                        = (AccessListICMPParams *)
                          RtParseMallocAndSet(sizeof(AccessListICMPParams));
                    newAccessList->icmpParameters->icmpType = atoi(token1);
                    newAccessList->icmpParameters->icmpCode = -1;
                }
                cutTokenCount = 1;
            }
        }
        else
        {
            // something wrong, if number it has to be a type or code
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nAccess list ICMP type in "
                "router config is not valid\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // some vague data entered
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nAccess list wrong parameters in"
            "router config file\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    if (cutTokenCount == 1)
    {
       currentPosition = RtParseGetPositionInString(*criteriaString, token1);
    }
    else if (cutTokenCount == 2)
    {
        currentPosition = RtParseGetPositionInString(*criteriaString,
            token2);
    }

    // update the string
    *criteriaString = *criteriaString + currentPosition;
    // now left is the common params for all protocols, so return
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListParseIGMPCriteria
// PURPOSE   Parse the name access list
// ARGUMENTS node, The node initializing
//           originalString, the original criteria
//           ip, ip data pointer
//           criteriaString, the remaining parse string
//           newAccessList, pointer to Access List
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListParseIGMPCriteria(Node* node,
                                 const char* originalString,
                                 NetworkDataIp* ip,
                                 char** criteriaString,
                                 AccessList* newAccessList)
{
    char token1[MAX_STRING_LENGTH];
    int cutTokenCount = 0;
    int currentPosition = 0;

    // parse the source address
    AccessListParseAddress(node,
                           originalString,
                           criteriaString,
                           newAccessList,
                           TRUE);

    if (**criteriaString == '\0')
    {
        // string has terminated abnoramlly, so an error,
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax format for "
            "Access list. Missing destination address.\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // parse destination address
    AccessListParseAddress(node,
                           originalString,
                           criteriaString,
                           newAccessList,
                           FALSE);

    if (**criteriaString == '\0')
    {
        // string has terminated so return
        return;
    }

    currentPosition = 0;
    cutTokenCount = 0;

    // optional igmp parameters
    // [ type | message ]
    sscanf(*criteriaString, "%s", token1);

    if (isalpha((int)*token1))
    {
        // is a string so match the name
        int type = 0;

        if (AccessListSearchIGMPNameValue(token1, ip, &type) == 1)
        {
            if (newAccessList->igmpParameters == NULL)
            {
                newAccessList->igmpParameters
                    = (AccessListIGMPParams *)
                      RtParseMallocAndSet(sizeof(AccessListIGMPParams));
                newAccessList->igmpParameters->igmpType =
                    (unsigned char)type;
            }
            cutTokenCount = 1;
        }
    }
    else if (isdigit((int)*token1))
    {
        // is a number so match the type
        if (AccessListIsIGMPType(atoi(token1),ip) == TRUE)
        {
            if (newAccessList->igmpParameters == NULL)
            {
                newAccessList->igmpParameters
                    = (AccessListIGMPParams *)
                      RtParseMallocAndSet(sizeof(AccessListIGMPParams));
                newAccessList->igmpParameters->igmpType = (unsigned char)
                    atoi(token1);
            }
            cutTokenCount = 1;
        }
        else
        {
            // is error, if number, has to be igmp type
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nAccess list wrong parameters in"
                "router config file\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // some vague data entered
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nAccess list wrong parameters in"
            " router config file\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    if (cutTokenCount == 1)
    {
        currentPosition = RtParseGetPositionInString(
            *criteriaString, token1);
    }

    // update the string
    *criteriaString = *criteriaString + currentPosition;
    // now left is the common params for all protocols, so return
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListFillReflexiveParameters
// PURPOSE   Initialize the reflex access list parameters
// ARGUMENTS newAccessList, pointer to Access List
//           accessList, the defined access list
//           msg, packet thats recieved
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListFillReflexiveParameters(AccessList* newAccessList,
                                       AccessList* accessList,
                                       const Message* msg)
{
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    newAccessList->protocol = ipHeader->ip_p;
    newAccessList->srcAddr = ipHeader->ip_dst;
    newAccessList->srcMaskAddr = ~(ANY_DEST);
    newAccessList->destAddr = ipHeader->ip_src;
    newAccessList->destMaskAddr = ~(ANY_DEST);
    newAccessList->filterType = accessList->filterType;


    // we need to hold the time for removal of the reflexive access list
    newAccessList->reflexParams =
        (AccessListReflex *) RtParseMallocAndSet(sizeof(AccessListReflex));
    newAccessList->reflexParams->lastPacketSent = 0;
    newAccessList->reflexParams->countFIN = 0;

    if (accessList->reflexParams->timeout != -1)
    {
        newAccessList->reflexParams->timeout =
            accessList->reflexParams->timeout;
    }
    else
    {
        newAccessList->reflexParams->timeout = -1;
    }

    // we will reference the values of ICMP and IGMP structure, since
    //  they remain the same
    if (accessList->icmpParameters)
    {
        newAccessList->icmpParameters = accessList->icmpParameters;
    }
    else if (accessList->igmpParameters)
    {
        newAccessList->igmpParameters = accessList->igmpParameters;
    }
    else
    {
        // if the packet is not ICMP or IGMP, we check whether the
        //  source packet is TCP or UDP
        if (ipHeader->ip_p == IPPROTO_TCP || ipHeader->ip_p == IPPROTO_UDP)
        {
            int ipHdrSize = IpHeaderSize(ipHeader);
            char* pktPtr = (char*)(MESSAGE_ReturnPacket(msg) + ipHdrSize);
            struct tcphdr* tcpHeader = (struct tcphdr*) pktPtr;

            // if we have tcp params initialized, lets reverese this
            newAccessList->tcpOrUdpParameters
                = (AccessListTCPorUDPParams *)
                RtParseMallocAndSet(sizeof(AccessListTCPorUDPParams));

            newAccessList->tcpOrUdpParameters->srcPortNumber =
                tcpHeader->th_dport;
            newAccessList->tcpOrUdpParameters->srcOperator =
                ACCESS_LIST_EQUALITY;

            newAccessList->tcpOrUdpParameters->destPortNumber =
                tcpHeader->th_sport;
            newAccessList->tcpOrUdpParameters->destOperator =
                ACCESS_LIST_EQUALITY;
        }
    }

    newAccessList->isDynamicallyCreated = TRUE;
    newAccessList->isProxy = FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION   AccessListSetTimer
// PURPOSE    Set timer for log
// PARAMETERS node, this node.
//            eventType, type of event
//            timerType, type of timer
//            delay, delay for message trigger
//            accessList, access list pointer
//            id, session id associated with this access list
// RETURN     None
//--------------------------------------------------------------------------
static
void AccessListSetTimer(Node* node,
                        Int32 eventType,
                        AccessListTimerType timerType,
                        clocktype delay,
                        AccessList* accessList,
                        int id)
{
    AccessListMessageInfo* info =NULL;

    Message* msg = MESSAGE_Alloc(node,
                                 NETWORK_LAYER,
                                 NETWORK_PROTOCOL_IP,
                                 eventType);

    MESSAGE_InfoAlloc(node, msg, sizeof(AccessListMessageInfo));

    info = (AccessListMessageInfo*) MESSAGE_ReturnInfo(msg);
    info->accessList = accessList;
    info->timerType = timerType;
    info->sessionId = id;

    memcpy(MESSAGE_ReturnInfo(msg), info, sizeof(AccessListMessageInfo));

    MESSAGE_Send(node, msg, delay);
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListCreateReflex
// PURPOSE   Initialize the reflex access list parameters
// ARGUMENTS newAccessList, pointer to Access List
//           accessList, the defined access list
//           msg, packet thats recieved
//           interfaceType, type of interface
//           interfaceId, interface index
// RETURN    The newly created reflex access list
//--------------------------------------------------------------------------
static
AccessList* AccessListCreateReflex(Node* node,
                                   AccessList* accessList,
                                   const Message* msg)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    AccessListNest* nestReflex = ip->nestReflex;
    AccessListReflex* reflexParam = accessList->reflexParams;
    AccessListName* accessListName = ip->accessListName;
    AccessList* tempAccessList = NULL;
    AccessList* newAccessList = NULL;
    char* reflexDefName = NULL;
    char* nestAccessListName = NULL;
    int position = 0;
    BOOL isMatch = FALSE;
    BOOL isFound = FALSE;

    if (nestReflex == NULL)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nReflex access list not nested."
            "No dynamic entry created for reflex access list.\n",
            node->nodeId);
        ERROR_ReportWarning(errString);
        return NULL;
    }

    if (reflexParam == NULL)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nReflex access list not defined."
            "No dynamic entry created for reflex access list.\n",
            node->nodeId);
        ERROR_ReportWarning(errString);
        return NULL;
    }

    reflexDefName = RtParseStringAllocAndCopy(reflexParam->name);

    while (nestReflex != NULL)
    {
        if (RtParseStringNoCaseCompare(
            reflexDefName, nestReflex->name) == 0)
        {
            isMatch = TRUE;
            break;
        }
        nestReflex = nestReflex->next;
    }

    if (isMatch == FALSE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nReflex access list not nested."
            "No dynamic entry created for reflex access list %s.\n",
            node->nodeId, reflexDefName);
        ERROR_ReportWarning(errString);
        return NULL;
    }

    if (nestReflex)
    {
        position = nestReflex->position;
        nestAccessListName = RtParseStringAllocAndCopy(
            nestReflex->accessListName);
    }

    if (accessListName == NULL)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nNo correponding name access list"
            "associated. Dynamic creation of reflexive access list aborted"
            "\n", node->nodeId);
        ERROR_ReportWarning(errString);
        return NULL;
    }

    while (accessListName != NULL)
    {
        if (RtParseStringNoCaseCompare(accessListName->name,
                                          nestAccessListName) == 0)
        {
            isFound = TRUE;
            break;
        }
        accessListName = accessListName->next;
    }

    if (isFound == FALSE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nNo correponding name access list"
            "associated. Dynamic creation of reflexive access list aborted"
            "\n", node->nodeId);
        ERROR_ReportWarning(errString);
        return NULL;
    }

    // insert the dynamic access list at the right position
    // get the existing access lists from the name
    tempAccessList = accessListName->accessList;

    newAccessList = AccessListCreateTemplate();
    newAccessList->accessListName = RtParseStringAllocAndCopy(
        reflexDefName);

    if (position == 1)
    {
        newAccessList->next = tempAccessList;
        accessListName->accessList = newAccessList;
    }
    else
    {
        int count = 1;
        AccessList* prevAccessList = NULL;

        while (count != position)
        {
            // we will loop till the right count has come
            prevAccessList = tempAccessList;
            tempAccessList = tempAccessList->next;
            count++;
        }

        ERROR_Assert(prevAccessList != NULL, "Preceding access list cant"
                " be NULL.\n");

        prevAccessList->next = newAccessList;
        newAccessList->next = tempAccessList;
    }

    // fill in the list
    AccessListFillReflexiveParameters(newAccessList, accessList, msg);

    // insert the position in the list for the new access list
    newAccessList->reflexParams->position = position;

    // fill the time
    if (newAccessList->reflexParams->timeout == -1)
    {
        if (ip->reflexTimeout != -1)
        {
            newAccessList->reflexParams->timeout = ip->reflexTimeout;
        }
        else
        {
            newAccessList->reflexParams->timeout =
                ACCESS_LIST_DEFAULT_REFLEX_TIMEOUT;
        }
    }

    MEM_free(reflexDefName);
    MEM_free(nestAccessListName);

    return newAccessList;
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListDefineReflex
// PURPOSE   Initialize the reflex access list
// ARGUMENTS node, current node
//           accessList, pointer to Access List
//           name, the name of reflex access list
//           time, timeout value
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListDefineReflex(Node* node,
                            AccessList* accessList,
                            char* name,
                            char* time)
{
    AccessListReflex* tempReflex = accessList->reflexParams;
    AccessListReflex* newReflex
        = (AccessListReflex*) RtParseMallocAndSet(sizeof(AccessListReflex));

    if (tempReflex == NULL)
    {
        // first reflex
        accessList->reflexParams = newReflex;
    }
    else
    {
        while (tempReflex->next != NULL)
        {
            tempReflex = tempReflex->next;
        }

        tempReflex->next = newReflex;
    }

    newReflex->name = RtParseStringAllocAndCopy(name);

    if (time == NULL)
    {
        newReflex->timeout = -1;
    }
    else
    {
        newReflex->timeout = TIME_ConvertToClock(time);

        if ((newReflex->timeout < ACCESS_LIST_MIN_REFLEX_TIMEOUT) ||
            (newReflex->timeout > TIME_ReturnMaxSimClock(node)))
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "Individual timeout time should not"
                "be less than 0 and greter than max simulation time"
                ".\n'%s'\n", name);
            ERROR_ReportError(errString);
        }
    }
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListParseCriteria
// PURPOSE   Parse the name access list
// ARGUMENTS node, The node initializing
//           originalString, the original criteria
//           ip, ip data pointer
//           criteriaString, the remaining parse string
//           newAccessList, pointer to Access List
//           isStandard, true if standard or false for extended
// RETURN    None
// COMMENT   Parses from filter onwards. Called by both name and number
//              access list.
//--------------------------------------------------------------------------
static
void AccessListParseCriteria(Node* node,
                             const char* originalString,
                             NetworkDataIp* ip,
                             char** criteriaString,
                             AccessList* newAccessList,
                             BOOL isStandard)
{
    char token1[MAX_STRING_LENGTH];
    char token2[MAX_STRING_LENGTH];
    char token3[MAX_STRING_LENGTH];
    char token4[MAX_STRING_LENGTH];
    char token5[MAX_STRING_LENGTH];

    int numInput = 0;
    int currentPosition = 0;

    // hold whether std or extd for stats
    newAccessList->isStandard = isStandard;
    newAccessList->isProxy = FALSE;

    // extracting filter type
    sscanf(*criteriaString, "%s", token1);
    if (RtParseStringNoCaseCompare(token1, "PERMIT") == 0)
    {
        newAccessList->filterType = ACCESS_LIST_PERMIT;
    }
    else if (RtParseStringNoCaseCompare(token1, "DENY") == 0)
    {
        newAccessList->filterType = ACCESS_LIST_DENY;
    }
    else if (RtParseStringNoCaseCompare(token1, "REMARK") == 0)
    {
        // for commented access lists
        currentPosition = RtParseGetPositionInString(
            *criteriaString, token1);

        *criteriaString = *criteriaString + currentPosition;

        if (**criteriaString == '\0')
        {
            // this is not allowed
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node %u\nMissing commment after 'remark'"
                ".\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
        else
        {
            AccessList* tempAccessList = NULL;
            AccessList* prevAccessList = NULL;
            char accessListId[MAX_STRING_LENGTH] = {0};

            if (newAccessList->accessListName)
            {
                strcpy(accessListId, newAccessList->accessListName);
            }
            else
            {
                sprintf(accessListId,"%d", newAccessList->accessListNumber);
            }

            // this has to be only number access list
            if (isalpha((int)*accessListId))
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "node %u\nREMARK assigned to name "
                    " access list. Wrong syntax for REMARK. \n'%s'\n",
                    node->nodeId, originalString);
                ERROR_ReportError(errString);
            }

            tempAccessList = ip->accessList[atoi(accessListId) - 1];
            if (!tempAccessList)
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "node %u\nAccess list ID not intialized"
                    " or invlid entry for REMARK. \n'%s'\n", node->nodeId,
                    originalString);
                ERROR_ReportError(errString);
            }

            while (tempAccessList != NULL)
            {
                prevAccessList = tempAccessList;
                tempAccessList = tempAccessList->next;
            }

            strncpy(prevAccessList->remark, *criteriaString,
                ACCESS_LIST_REMARK_STRING_SIZE);
            prevAccessList->continueNextLine = TRUE;
        }
        return;
    }
    else
    {
        // this filter is not permissible
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u\naccess list filter can be either"
            "  PERMIT or DENY.\n'%s'\n", node->nodeId,
            originalString);
        ERROR_ReportError(errString);
    }

    // update the criteria string
    currentPosition = RtParseGetPositionInString(
        *criteriaString, token1);
    *criteriaString = *criteriaString + currentPosition;

    if (**criteriaString == '\0')
    {
        // string has terminated abnoramlly, so an error,
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax format for "
            "Access list. Missing source address.\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    currentPosition = 0;
    if (isStandard == TRUE)
    {
        // check the syntax for the standard access list
        sscanf(*criteriaString, "%s %s", token1, token2);

        // parsing for Standard Access List
        // format left: source|source wildcard
        // just parse the source wildcard
        AccessListParseAddress(node,
                               originalString,
                               criteriaString,
                               newAccessList,
                               TRUE);

        if (**criteriaString == '\0')
        {
            return;
        }
        // gotto check for log
        sscanf(*criteriaString, "%s", token1);

        if (RtParseStringNoCaseCompare(token1, "LOG") == 0)
        {
            newAccessList->logType = ACCESS_LIST_LOG;
            return;
        }
        else if (RtParseStringNoCaseCompare(token1, "LOG-INPUT") == 0)
        {
            newAccessList->logType = ACCESS_LIST_LOG_INPUT;
            return;
        }
        else
        {
            // this is junk
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node %u\n Standard access list not having"
                " proper syntax.\n'%s'\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
    }

    // considered that we are going ahead with Extended AccessList
    // crtieria parameter varies as per protocol, so we channelize
    {
        int currentPosition;
        sscanf(*criteriaString, "%s", token1);
        currentPosition = RtParseGetPositionInString(
            *criteriaString, token1);

        // update to the next token count
        *criteriaString = *criteriaString + currentPosition;
        if (**criteriaString == '\0')
        {
            // string has terminated abnoramlly, so an error,
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax format for "
                "Access list. Missing Protocol field.\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
    }

    if (RtParseStringNoCaseCompare(token1, "TCP") == 0)
    {
        newAccessList->protocol = IPPROTO_TCP;
        AccessListParseTCPorUDPCriteria(node,
                                        originalString,
                                        ip,
                                        criteriaString,
                                        newAccessList);
    }
    else if (RtParseStringNoCaseCompare(token1, "UDP") == 0)
    {
        newAccessList->protocol = IPPROTO_UDP;
        AccessListParseTCPorUDPCriteria(node,
                                        originalString,
                                        ip,
                                        criteriaString,
                                        newAccessList);
    }
    else if (RtParseStringNoCaseCompare(token1, "ICMP") == 0)
    {
        newAccessList->protocol = IPPROTO_ICMP;
        AccessListParseICMPCriteria(node,
                                    originalString,
                                    ip,
                                    criteriaString,
                                    newAccessList);
    }
    else if (RtParseStringNoCaseCompare(token1, "IGMP") == 0)
    {
        newAccessList->protocol = IPPROTO_IGMP;
        AccessListParseIGMPCriteria(node,
                                    originalString,
                                    ip,
                                    criteriaString,
                                    newAccessList);
    }
    else if (RtParseStringNoCaseCompare(token1, "IP") == 0)
    {
        newAccessList->protocol = IPPROTO_IP;
        AccessListParseTCPorUDPCriteria(node,
                                        originalString,
                                        ip,
                                        criteriaString,
                                        newAccessList);
    }
    // can add more protocols if needed
    else
    {
        // protocol not supported
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u\n protocol's supported are TCP,"
            "UDP, ICMP and IGMP.\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // three optional params left
    //['precedence' precedence]['tos' tos][log | log-input]

    // check whether any params exist, to avoid a costly search below
    if (**criteriaString == '\0')
    {
        // no more params
        return;
    }

    numInput = sscanf(*criteriaString, "%s %s %s %s %s",
        token1, token2, token3, token4, token5);

    // analyze the reflex access list
    if (RtParseStringNoCaseCompare(token1, "REFLECT") == 0)
    {
        if (isStandard == TRUE)
        {
            // reflexive access list not allowed for standard access list
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for "
                "REFLEX access list in router config file. Reflex access "
                "list not applicable for standard access list.\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        if (newAccessList->filterType == ACCESS_LIST_DENY)
        {
            // reflexive access list does not allow deny criteria
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for "
                "REFLEX access list in router config file. Reflex access "
                "list not applicable for 'DENY' filter.\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        if (newAccessList->accessListNumber != 0)
        {
            // reflexive access list not applicable for numbered access list
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for "
                "REFLEX access list in router config file. Reflex access "
                "list not applicable for numbered access list.\n'%s'\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        switch (numInput)
        {
            case 4:
            {
                // check up whether we have timeout
                if (!RtParseStringNoCaseCompare(token3, "TIMEOUT"))
                {
                    if (isdigit((int)*token4))
                    {
                        AccessListDefineReflex(node,
                                               newAccessList,
                                               token2,
                                               token4);
                        newAccessList->isReflectDefined = TRUE;
                    }
                    else
                    {
                        char errString[MAX_STRING_LENGTH];
                        sprintf(errString, "\nNode :%u\nWrong syntax for "
                            "REFLEX access list in router config file. "
                            "Expecting TIMEOUT value.\n'%s'\n",
                            node->nodeId, originalString);
                        ERROR_ReportError(errString);
                    }
                }
                else
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "\nNode :%u\nWrong syntax for "
                        "REFLEX access list in router config file. "
                        "Expecting 'TIMEOUT'.\n'%s'\n",
                        node->nodeId, originalString);
                    ERROR_ReportError(errString);
                }

                break;
            }
            case 3:
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "\nNode :%u\nWrong syntax for "
                    "REFLEX access list in router config file. Expecting "
                    "TIMEOUT value.\n'%s'\n", node->nodeId, originalString);
                ERROR_ReportError(errString);
                break;
            }
            case 2:
            {
                AccessListDefineReflex(node, newAccessList, token2, NULL);
                newAccessList->isReflectDefined = TRUE;

                // all over return
                break;
            }
            default:
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "\nNode :%u\nWrong syntax for "
                    "REFLEX access list in router config file. \n'%s'\n",
                    node->nodeId, originalString);
                ERROR_ReportError(errString);
                break;
            }
        }

        return;
        // this was reflex definitions, so return after its handling
    }

    // remaining parameters:
    // [precedence <precedence name | value>] [tos <tos name or value>]
    //  [log | log-input]
    switch (numInput)
    {
        case 5:
        {
            // all options are there
            if ((RtParseStringNoCaseCompare("precedence", token1) != 0)
                && (RtParseStringNoCaseCompare("tos", token3) != 0))
            {
                // this has to be precedence
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "\nNode :%u\nAccess list wrong syntax"
                    "for'precedence' or 'tos' in router config file\n'%s'"
                    "\n", node->nodeId, originalString);
                ERROR_ReportError(errString);
            }
            else
            {
                AccessListParsePrecedence(node,
                                          originalString,
                                          token2,
                                          newAccessList,
                                          ip);

                AccessListParseTOS(node,
                                   originalString,
                                   token4,
                                   newAccessList,
                                   ip);

                AccessListParseLog(node, originalString, token5,
                    newAccessList);
            }
            break;
        }
        case 4:
        {
            // precedence and TOS and no log
            if ((RtParseStringNoCaseCompare("precedence", token1) != 0)
                && (RtParseStringNoCaseCompare("tos", token3) != 0))
            {
                // this has to be precedence
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "\nNode :%u\nAccess list wrong syntax in"
                    " router config file\n'%s'\n", node->nodeId,
                    originalString);
                ERROR_ReportError(errString);
            }
            else
            {
                AccessListParsePrecedence(node,
                                          originalString,
                                          token2,
                                          newAccessList,
                                          ip);

                AccessListParseTOS(node,
                                   originalString,
                                   token4,
                                   newAccessList,
                                   ip);
            }
            break;
        }
        case 3:
        {
            // either precedence or TOS and log
            if ((RtParseStringNoCaseCompare("precedence", token1) == 0))
            {
                AccessListParsePrecedence(node,
                                          originalString,
                                          token2,
                                          newAccessList,
                                          ip);

                AccessListParseLog(node,
                                   originalString,
                                   token3,
                                   newAccessList);
            }
            else if ((RtParseStringNoCaseCompare("tos", token1) == 0))
            {
                AccessListParseTOS(node,
                                   originalString,
                                   token2,
                                   newAccessList,
                                   ip);

                AccessListParseLog(node,
                                   originalString,
                                   token3,
                                   newAccessList);
            }
            else
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "\nNode :%u\nAccess list wrong syntax in"
                    " router config file\n'%s'\n", node->nodeId,
                    originalString);
                ERROR_ReportError(errString);
            }
            break;
        }
        case 2:
        {
            // either tos or precedence
            if ((RtParseStringNoCaseCompare("precedence", token1) == 0))
            {
                AccessListParsePrecedence(node,
                                          originalString,
                                          token2,
                                          newAccessList,
                                          ip);
            }
            else if ((RtParseStringNoCaseCompare("tos", token1) == 0))
            {
                AccessListParseTOS(node,
                                   originalString,
                                   token2,
                                   newAccessList, ip);
            }
            else
            {
                char errString[MAX_STRING_LENGTH];
                sprintf(errString, "\nNode :%u\nAccess list wrong syntax in"
                    " router config file\n'%s'\n", node->nodeId,
                    originalString);
                ERROR_ReportError(errString);
            }
            break;
        }
        case 1:
        {
            AccessListParseLog(node, originalString, token1,
                newAccessList);
            break;
        }
        default:
        {
            // this is an error
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nAccess list wrong syntax in"
                " router config file\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
            break;
        }
        // handling of the criteria statement is over, so return
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListParseNameCriteria
// PURPOSE   Parse the name access list
// ARGUMENTS node, The node initializing
//           originalString, the original criteria
//           ip, ip data pointer
//           criteriaString, the remaining parse string
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListParseNameCriteria(Node* node,
                                 const char* originalString,
                                 NetworkDataIp* ip,
                                 char** criteriaString)
{
    AccessListName* newAccessListName = NULL;
    AccessListName* headAccessListName = NULL;
    AccessListName* bufferAccessListName = NULL;

    AccessList *newAccessList = NULL;
    AccessList *tempAccessList = NULL;
    char accessListName[MAX_STRING_LENGTH];
    char accessListType[MAX_STRING_LENGTH];
    int currentPosition = 0;
    BOOL isSameName = FALSE;
    BOOL isStandard = FALSE;

    sscanf(*criteriaString, "%s %s", accessListType, accessListName);

    // what type of access list
    if (RtParseStringNoCaseCompare(accessListType, "STANDARD") == 0)
    {
        isStandard = TRUE;
    }
    else if (RtParseStringNoCaseCompare(accessListType, "EXTENDED") == 0)
    {
        isStandard = FALSE;
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u\naccess list type should be "
            "either 'standard' or 'extended'\n'%s'\n", node->nodeId,
            originalString);
        ERROR_ReportError(errString);
    }

    headAccessListName = ip->accessListName;

    if (headAccessListName == NULL)
    {
        // if name list is empty
        newAccessListName = (AccessListName*) RtParseMallocAndSet(sizeof(
            AccessListName));
        newAccessList = AccessListCreateTemplate();
        newAccessListName->accessList = newAccessList;
        ip->accessListName = newAccessListName;

        newAccessListName->name = (char*) RtParseMallocAndSet(strlen(
            accessListName) + 1);
        strcpy(newAccessListName->name, accessListName);

        newAccessListName->isStandardType = isStandard;

        newAccessList->accessListName = (char*) RtParseMallocAndSet(strlen(
            accessListName) + 1);
    }
    else
    {
        while (headAccessListName != NULL)
        {
            if (RtParseStringNoCaseCompare(headAccessListName->name,
                                              accessListName) == 0)
            {
                // if name exists
                // plug the access list in the end
                BOOL isBreak = FALSE;
                AccessList* prevAccessList = NULL;
                tempAccessList = headAccessListName->accessList;

                while (tempAccessList != NULL)
                {
                    if (tempAccessList->continueNextLine)
                    {
                        isBreak = TRUE;
                        tempAccessList->continueNextLine = FALSE;
                        break;
                    }
                    prevAccessList = tempAccessList;
                    tempAccessList = tempAccessList->next;
                }

                if (!isBreak)
                {
                    newAccessList = AccessListCreateTemplate();
                    //newAccessList->isStandardType = isStandard;
                    prevAccessList->next = newAccessList;
                }
                else
                {
                    // access list already declared, so lets use the
                    //  existing access list
                    newAccessList = tempAccessList;
                }

                // check whether the criteria name has the same type
                if (headAccessListName->isStandardType != isStandard)
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "node :%u\nAccess list name has"
                        "type mismatch with previous decalaration.\n'%s'\n",
                        node->nodeId, originalString);
                    ERROR_ReportError(errString);
                }

                isSameName = TRUE;
                break;
            }
            bufferAccessListName = headAccessListName;
            headAccessListName = headAccessListName->next;
        }

        if (isSameName == FALSE)
        {
            // name didn't occur
            newAccessListName = (AccessListName*) RtParseMallocAndSet(sizeof(
                AccessListName));
            newAccessList = AccessListCreateTemplate();
            newAccessListName->accessList = newAccessList;
            bufferAccessListName->next =  newAccessListName;

            newAccessListName->name = (char*) RtParseMallocAndSet(strlen(
                accessListName) + 1);
            strcpy(newAccessListName->name, accessListName);
            newAccessListName->isStandardType = isStandard;
        }
    }

    newAccessList->accessListName = (char*) RtParseMallocAndSet(
        strlen(accessListName) + 1);
    strcpy(newAccessList->accessListName, accessListName);

    currentPosition = RtParseGetPositionInString(*criteriaString,
                                            accessListName);

    *criteriaString = *criteriaString + currentPosition;

    // for named access list
    if (**criteriaString == '\0')
    {
        newAccessList->continueNextLine = TRUE;
        return;
    }

    // from now on we will have generic formats for both name and number AL
    AccessListParseCriteria(node,
                            originalString,
                            ip,
                            criteriaString,
                            newAccessList,
                            isStandard);

    // for standard this returns after deny/permit or for just remark
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListParseNumberCriteria
// PURPOSE   Check a packet for matching with the access list
// ARGUMENTS node, The node initializing
//           originalString, the original criteria
//           ip, ip data pointer
//           criteriaString, the remaining parse string
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListParseNumberCriteria(Node* node,
                                   const char* originalString,
                                   NetworkDataIp* ip,
                                   char** criteriaString)
{
    int accessListNumber = -1;
    int currentPosition = 0;
    AccessList* accessListPointer = NULL;
    AccessList* newAccessList = NULL;

    char lastToken[MAX_STRING_LENGTH];
    char* tempToken = NULL;
    BOOL isStandard = FALSE;

    sscanf(*criteriaString, "%d", &accessListNumber);
    sscanf(*criteriaString, "%s", lastToken);
    tempToken = lastToken;

    while (*tempToken)
    {
        if (*tempToken == '.')
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nAccess list number should be "
                "between 1 and 199. Fraction not allowed.\n%s\n",
                node->nodeId, originalString);
            ERROR_ReportError(errString);
        }
        tempToken++;
    }

    if ((accessListNumber < ACCESS_LIST_MIN_STANDARD)
        || (accessListNumber > ACCESS_LIST_MAX_EXTENDED))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nAccess list number should be "
            "between 1 and 199\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    accessListPointer = ip->accessList[accessListNumber - 1];

    if (accessListPointer == NULL)
    {
        // no entry in this number
        newAccessList = AccessListCreateTemplate();
        ip->accessList[accessListNumber - 1] = newAccessList;
        newAccessList->accessListNumber = accessListNumber;
    }
    else
    {
        AccessList* prevAccessList = NULL;

        while (accessListPointer != NULL)
        {
            prevAccessList = accessListPointer;
            accessListPointer = accessListPointer->next;
        }

        if (!(prevAccessList->continueNextLine))
        {
            // initialize
            newAccessList = AccessListCreateTemplate();
            newAccessList->accessListNumber = accessListNumber;

            // attach to the last of the existing list
            prevAccessList->next = newAccessList;
        }
        else
        {
            prevAccessList->continueNextLine = FALSE;
            newAccessList = prevAccessList;
        }
    }

    currentPosition = RtParseGetPositionInString(*criteriaString, lastToken);

    // what type of access list
    // CISCO doc states standard access list number should be from 1 to
    //  99 and extended access list should be from 100 to 199.
    if ((accessListNumber > ACCESS_LIST_MIN_STANDARD - 1) &&
        (accessListNumber < ACCESS_LIST_MIN_EXTENDED))
    {
        isStandard = TRUE;
    }
    else if ((accessListNumber > ACCESS_LIST_MIN_EXTENDED - 1) &&
        (accessListNumber < ACCESS_LIST_MAX_EXTENDED + 1))
    {
        isStandard = FALSE;
    }

    *criteriaString = *criteriaString + currentPosition;
    // from now on we will have generic formats for both name and number AL
    AccessListParseCriteria(node,
                            originalString,
                            ip,
                            criteriaString,
                            newAccessList,
                            isStandard);
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListSetAddressForInterface
// PURPOSE   Set address for a given interface
// ARGUMENTS node, The node initializing
//           originalString, the original criteria
//           criteriaString, the remaining parse string
//           interfaceIndex, return matching interface
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListSetAddressForInterface(Node* node,
                                      char** criteriaString,
                                      char* originalString,
                                      int* interfaceIndex)
{
    char token1[MAX_STRING_LENGTH];
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    BOOL isNodeId = FALSE;

    NodeAddress address = ~ANY_DEST;
    NodeAddress subnetMask = ~ANY_DEST;
    int i = 0;
    int currentPosition = 0;
    BOOL end = FALSE;

    sscanf(*criteriaString, "%s", token1);
    IO_ParseNodeIdOrHostAddress(token1, &address, &isNodeId);

    if (isNodeId)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax for node address "
            "in router config file\n'%s'\n",node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    currentPosition = RtParseGetPositionInString(*criteriaString, token1);

    *criteriaString = *criteriaString + currentPosition;
    currentPosition = 0;

    // if we have the mask
    if (**criteriaString == '\0')
    {
        end = TRUE;
    }

    if (!end)
    {
        sscanf(*criteriaString, "%s", token1);
        IO_ParseNodeIdOrHostAddress(token1, &subnetMask, &isNodeId);

        if (isNodeId)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for node address "
                "in router config file\n'%s'\n",node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }

        currentPosition = RtParseGetPositionInString(*criteriaString,
            token1);

        *criteriaString = *criteriaString + currentPosition;
        currentPosition = 0;
    }

    // for secondary address
    if (**criteriaString == '\0')
    {
        end = TRUE;
    }

    if (end == FALSE)
    {
        sscanf(*criteriaString, "%s", token1);

        if (RtParseStringNoCaseCompare(token1, "SECONDARY") != 0)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nNode :%u\nWrong syntax for Address "
                "assignment for interface in router config file\n Expected"
                " 'SECONDARY' keyword.\n'%s'\n", node->nodeId,
                originalString);
            ERROR_ReportError(errString);
        }
        currentPosition = RtParseGetPositionInString(*criteriaString, token1);

        *criteriaString = *criteriaString + currentPosition;
    }


    if (**criteriaString != '\0')
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nWrong syntax for Address "
            "assignment for interface in router config file\n Unexpected"
            " tokens used.\n'%s'\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // we dont need this ...
    //address = address & subnetMask;

    // return the interface
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ip->interfaceInfo[i]->ipAddress == address)
        {
            *interfaceIndex = i;
            break;
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListProcessInterfaceStatement
// PURPOSE   Parse the INTERFACE statement
//           Ex: INTERFACE 0
// ARGUMENTS node, The node initializing
//           rtConfig, Router configuration file
//           index, line number of Router config file
//           interfaceName, if name then return the name
//           isIndex, interface is entered as number or name
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListProcessInterfaceStatement(Node* node,
                                         const NodeInput rtConfig,
                                         int index,
                                         char* interfaceName,
                                         BOOL* isIndex)
{
    char input[MAX_STRING_LENGTH] = {0};
    int numInput = sscanf(rtConfig.inputStrings[index], "%*s %s", input);

    if (numInput != 1)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u: Wrong router config file\n"
            "Error at statement #index\n", node->nodeId);
        ERROR_ReportError(errString);
    }

    // check whether index is number or name
    if (isdigit((int)input[0]))
    {
        *isIndex = TRUE;
    }
    else if (isalpha((int)input[0]))
    {
        interfaceName = RtParseStringAllocAndCopy(input);
        *isIndex = FALSE;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListProcessIpAccessGroupStatement
// PURPOSE   Process the access group statement
// ARGUMENTS node, The node initializing
//           originalString, the original criteria string
//           ip, ip data pointer
//           rtConfig, Router configuration file
//           index, line number of Router config file
//           interfaceIndex, interface number
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListProcessIpAccessGroupStatement(Node* node,
                                             char* originalString,
                                             NetworkDataIp* ip,
                                             const NodeInput rtConfig,
                                             int index,
                                             int interfaceIndex)
{
    int numInput = 0;
    char accessListId[MAX_STRING_LENGTH];
    char inOrOut[MAX_STRING_LENGTH];

    AccessListPointer* newAccessList = NULL;
    AccessListPointer* accessListPointer = NULL;
    AccessListName* nameAccessList = NULL;

    numInput = sscanf(rtConfig.inputStrings[index], "%*s %*s %s %s",
        accessListId, inOrOut);

    // Access group format expects IP ACCESS-GROUP 'Access-Number' IN/OUT
    if (numInput > 2)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nnode :%u\nCorrect form of ip access "
            "group is: ip access-group access-list-number "
            "in | out.\n%s\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    if (interfaceIndex == -1)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nInterface statement missing."
            "\n%s\n", node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // if access list number
    if (isdigit((int)*accessListId))
    {
        // CISCO says Access List number gotto be between 1 and 199
        if ((atoi(accessListId) < 1) || (atoi(accessListId) > 199))
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nAccess list should be "
                "between 1 and 199.\n%s\n", node->nodeId, originalString);
            ERROR_ReportError(errString);
        }

        // the list number should be iniatialized  earlier
        if (ip->accessList[atoi(accessListId) - 1] == NULL)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nAccess list %s not specified "
                "earlier.\n%s\n", node->nodeId, accessListId,
                originalString);
            ERROR_ReportError(errString);
        }

        newAccessList = (AccessListPointer*) RtParseMallocAndSet(
                             sizeof(AccessListPointer));
        newAccessList->accessList = &ip->accessList[atoi(accessListId) - 1];
        newAccessList->next = NULL;
    }
    else if (isalpha((int)*accessListId))
    {
        BOOL isFound = FALSE;
        // access list name mentioned
        nameAccessList = ip->accessListName;
        while (nameAccessList != NULL)
        {
            if (RtParseStringNoCaseCompare(nameAccessList->name,
                accessListId) == 0)
            {
                newAccessList = (AccessListPointer*) RtParseMallocAndSet(
                    sizeof(AccessListPointer));
                newAccessList->accessList = &(nameAccessList->accessList);
                newAccessList->next = NULL;
                isFound = TRUE;
                break;
            }
            nameAccessList = nameAccessList->next;
        }

        if (isFound == FALSE)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\nAccess list %s not specified "
                "earlier.\n%s\n", node->nodeId, accessListId,
                originalString);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // junk entered
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\nAccess list group '%s' not correct "
            "name or number.\n%s\n", node->nodeId, accessListId,
            originalString);
        ERROR_ReportError(errString);
    }

    if (numInput == 1)
    {
        accessListPointer = ip->interfaceInfo[interfaceIndex]->
                                accessListOutPointer;

        if (accessListPointer == NULL)
        {
            ip->interfaceInfo[interfaceIndex]->accessListOutPointer =
                newAccessList;
        }
        else
        {
            while (accessListPointer->next != NULL)
            {
                accessListPointer = accessListPointer->next;
            }
            accessListPointer->next = newAccessList;
        }
        return;
    }

    // interface description
    if (RtParseStringNoCaseCompare("IN", inOrOut) == 0)
    {
        accessListPointer = ip->interfaceInfo[interfaceIndex]->
                                accessListInPointer;
        if (accessListPointer == NULL)
        {
            ip->interfaceInfo[interfaceIndex]->accessListInPointer =
                newAccessList;
        }
        else
        {
            while (accessListPointer->next != NULL)
            {
                accessListPointer = accessListPointer->next;
            }
            accessListPointer->next = newAccessList;
        }
    }
    else if (RtParseStringNoCaseCompare("OUT", inOrOut) == 0)
    {
        accessListPointer = ip->interfaceInfo[interfaceIndex]->
                                accessListOutPointer;

        if (accessListPointer == NULL)
        {
            ip->interfaceInfo[interfaceIndex]->accessListOutPointer =
                newAccessList;
        }
        else
        {
            while (accessListPointer->next != NULL)
            {
                accessListPointer = accessListPointer->next;
            }
            accessListPointer->next = newAccessList;
        }
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u\ncorrect form of ip access "
            "group is: ip access-group access-list-number "
            "in | out\n", node->nodeId);
        ERROR_ReportError(errString);
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListAssignRemarkToName
// PURPOSE   Assigning remark to named access list
// ARGUMENTS node, The node initializing
//           ip, ip data pointer
//           originalString, the original criteria
//           criteriaString, the remaining parse string
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListAssignRemarkToName(Node* node,
                                  char** criteriaString,
                                  char* originalString)
{
    AccessListName* tempNameAccessList = NULL;
    AccessList* tempAccessList = NULL;
    AccessList* prevAccessList = NULL;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    // it is assumed that the remark will be plugged in the last
    //  name access list
    // traverse to the last access list name
    tempNameAccessList = ip->accessListName;

    if (tempNameAccessList == NULL)
    {
        // no name access list exists, error
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u: No name access list initialized"
            " in router config file. Remark usage invalid.\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    while (tempNameAccessList->next != NULL)
    {
        tempNameAccessList = tempNameAccessList->next;
    }

    // traverse to the last access list
    tempAccessList = tempNameAccessList->accessList;

    if (!tempAccessList)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u: No name access list initialized"
            " in router config file. Remark usage invalid.\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }
    else
    {
        // prevAccessList = tempAccessList;
        while (tempAccessList != NULL)
        {
            prevAccessList = tempAccessList;
            tempAccessList = tempAccessList->next;
        }
    }

    if (prevAccessList->continueNextLine == TRUE)
    {
        strncpy(prevAccessList->remark, *criteriaString,
            ACCESS_LIST_REMARK_STRING_SIZE);
        prevAccessList->continueNextLine = TRUE;
    }
    else
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u: No name access list initialized"
            " in router config file. Remark usage invalid.\n'%s'\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListGetLastNameAccessList
// PURPOSE   Process the access group statement
// ARGUMENTS ip, ip data pointer
//           accessList, return value of pointer to access list
//           isStandard, return value of type of access list
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListGetLastNameAccessList(NetworkDataIp* ip,
                                     AccessList** accessList,
                                     BOOL* isStandard)
{
    AccessListName* nameAccessList = ip->accessListName;
    AccessListName* prevNameAccessList = NULL;
    AccessList* tempAccessList = NULL;

    while (nameAccessList != NULL)
    {
        prevNameAccessList = nameAccessList;
        nameAccessList = nameAccessList->next;
    }

    // we got the name, now the access list
    tempAccessList = prevNameAccessList->accessList;
    *isStandard = prevNameAccessList->isStandardType;
    while (tempAccessList != NULL)
    {
        *accessList = tempAccessList;
        tempAccessList = tempAccessList->next;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListNestReflex
// PURPOSE   Implement the nested reflexive access list
// ARGUMENTS node, concerned node
//           reflexName, name of relfex access list
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListNestReflex(Node* node,
                          char* reflexName,
                          char* originalString)
{
    int position = 0;
    BOOL isStandard = FALSE;
    BOOL hasProtocol = FALSE;
    char* accessListName = NULL;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    AccessListNest* newReflex
        = (AccessListNest *) RtParseMallocAndSet(sizeof(AccessListNest));
    AccessListNest* oldReflex = NULL;
    AccessListName* nameAccessList = ip->accessListName;
    AccessListName* prevNameAccessList = NULL;
    AccessList* tempAccessList = NULL;

    if (nameAccessList == NULL)
    {
        // error
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u:No name access list to next reflex "
            "access list in router config file.\n%s\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }
    // we take that its the last named access list
    while (nameAccessList != NULL)
    {
        prevNameAccessList = nameAccessList;
        nameAccessList = nameAccessList->next;
    }

    // grab the name of access list
    accessListName = RtParseStringAllocAndCopy(prevNameAccessList->name);

    // check the type of access list
    isStandard = prevNameAccessList->isStandardType;

    // nesting not allowed in standard access list
    if (isStandard == TRUE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u:Reflex access list nested to wrong"
            " type of access list in router config file. Nesting not "
            "allowed in standard access list.\n%s\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // we got the name, now the access list
    tempAccessList = prevNameAccessList->accessList;

    if (tempAccessList == NULL)
    {
        // error
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node %u:No name access list to next reflex "
            "access list in router config file.\n%s\n",
            node->nodeId, originalString);
        ERROR_ReportError(errString);
    }

    // for the exact position
    if (tempAccessList->protocol != 0)
    {
        hasProtocol = TRUE;
    }

    while (tempAccessList != NULL)
    {
        position++;
        tempAccessList = tempAccessList->next;
    }

    // for proper positioning of reflex
    if (position != 1 || hasProtocol)
    {
        position++;
    }

    // store the reflex info in ip
    oldReflex = ip->nestReflex;

    if (oldReflex == NULL)
    {
        ip->nestReflex = newReflex;
    }
    else
    {
        while (oldReflex->next != NULL)
        {
            oldReflex = oldReflex->next;
        }

        oldReflex->next = newReflex;
    }

    newReflex->accessListName = RtParseStringAllocAndCopy
        (accessListName);
    newReflex->name = RtParseStringAllocAndCopy(reflexName);
    newReflex->position = position;
    newReflex->next = NULL;
    MEM_free(accessListName);
}



//--------------------------------------------------------------------------
// FUNCTION    AccessListStatInit
// PURPOSE     Initialize statistics for the access list
// ARGUMENTS   node, node concerned
//             nodeInput,
// RETURN      None
//--------------------------------------------------------------------------
static
void AccessListStatInit(Node* node,
                        const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ACCESS-LIST-STATISTICS",
        &retVal,
        buf);

    if (retVal == FALSE)
    {
        ip->isACLStatOn = FALSE;
    }
    else
    {
        if (strcmp(buf, "YES") == 0)
        {
            ip->isACLStatOn = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            ip->isACLStatOn = FALSE;
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node %u: Expecting YES or NO for"
                "ACCESS-LIST-STATISTICS parameter in configuration "
                "file.\n", node->nodeId);
            ERROR_ReportError(errString);
        }
    }
}



//--------------------------------------------------------------------------
// FUNCTION    AccessListTraceInit
// PURPOSE     Initialize trace for the access list
// ARGUMENTS   node, node concerned
//             nodeInput
// RETURN      None
//--------------------------------------------------------------------------
static
void AccessListTraceInit(Node* node,
                         const NodeInput* nodeInput)
{
    char yesOrNo[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    IO_ReadString(node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ACCESS-LIST-TRACE",
        &retVal,
        yesOrNo);

    if (retVal == TRUE)
    {
        if (!strcmp(yesOrNo, "NO"))
        {
            ip->accessListTrace = FALSE;
        }
        else if (!strcmp(yesOrNo, "YES"))
        {
            ip->accessListTrace = TRUE;
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node %u: AccessListTraceInit: "
                "Unknown value of ACCESS-LIST-TRACE in configuration "
                "file.\n", node->nodeId);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        // ACCESS-LIST-TRACE not enabled.
        ip->accessListTrace = FALSE;
    }
}


//--------------------------------------------------------------------------
//  FUNCTION  AccessListInit
//  PURPOSE   To initialize access list reading from router
//            configuration file
//  ARGUMENTS node, The node initializing
//            nodeInput, Main configuration file
//  RETURN    None
//  NOTE      The router configuration file should be in the following
//              format:
//            ROUTER <node id1> # followed by configuration for node
//              <node id1>
//            ACCESS-LIST <access list number> .....
//            IP ACCESS-GROUP <access list number> .....
//            ROUTER <node id1> # followed by configuration for node
//              <node id1>
// COMMENTS  Statements which are not used will be not parsed. No feedback
//              is given to the user.
//--------------------------------------------------------------------------
void AccessListInit(Node* node, const NodeInput* nodeInput)
{
    NodeInput rtConfig;
    BOOL isFound = FALSE;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    ip->accessList = NULL;
    ip->accessListName = NULL;
    ip->allNameValueLists = NULL;

    // Read configuration for router configuration file

    IO_ReadCachedFile(node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTER-CONFIG-FILE",
        &isFound,
        &rtConfig);

    // file found
    if (isFound == TRUE)
    {
        // This node may have some entry for router configuration.
        // So we need to go through all the entries in the file
        int count = 0;
        int interfaceIndex = -1;
        char* interfaceName = NULL;
        BOOL isIndex = FALSE;
        BOOL foundRouterConfiguration = TRUE;
        BOOL gotForThisNode = FALSE;

        //AL format followed:-
        //ROUTER 3 ( this is optional for router specific files)
        //ACCESS-LIST 1 ACCESS_LIST_DENY 0.0.1.2 0.0.0.0
        //INTERFACE 0
        //IP ACCESS-GROUP 1 IN

        // Initialize the various name value pairs before
        // parsing the criteria's
        AccessListInitNameValue(ip);

        // Initialize Access list in the router
        ip->accessList = (AccessList**) RtParseMallocAndSet(
            ACCESS_LIST_MAX_NUM * sizeof(AccessList*));

        for (count = 0; count < rtConfig.numLines; count++)
        {
            char* originalString = NULL;
            char* criteriaString = NULL;
            char* refCriteriaString = NULL;
            char token1[MAX_STRING_LENGTH] = {0};
            char token2[MAX_STRING_LENGTH] = {0};
            char token3[MAX_STRING_LENGTH] = {0};
            char token4[MAX_STRING_LENGTH] = {0};

            // find the first and second token
            sscanf(rtConfig.inputStrings[count],"%s %s %s %s",
                token1, token2, token3, token4);

           // we make a copy of the original string for error reporting
            originalString = RtParseStringAllocAndCopy(
                rtConfig.inputStrings[count]);

            if (RtParseStringNoCaseCompare("NODE-IDENTIFIER", token1) == 0)
            {
                BOOL isAnother = FALSE;
                foundRouterConfiguration = FALSE;

                RtParseIdStmt(
                    node,
                    rtConfig,
                    count,
                    &foundRouterConfiguration,
                    &gotForThisNode,
                    &isAnother);

                MEM_free(originalString);
                if (isAnother)
                {
                    // Got another router statement so this router's
                    // configuration is complete
                    break;
                }
                else
                {
                    continue;
                }
            }
            else if (foundRouterConfiguration &&
                (RtParseStringNoCaseCompare("ACCESS-LIST", token1) == 0))
            {
                // got one entry for numbered access list
                int currentPosition;

                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token1);

                criteriaString = RtParseStringAllocAndCopy(
                    originalString + currentPosition);

                refCriteriaString = criteriaString;

                AccessListParseNumberCriteria(node,
                                              originalString,
                                              ip,
                                              &refCriteriaString);

                MEM_free(criteriaString);
                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration
                && (RtParseStringNoCaseCompare("EVALUATE", token1) == 0))
            {
                int currentPosition = 0;

                // next the reflexive access list
                AccessListNestReflex(node, token2, originalString);

                // check whether some more tokens exist, wrong syntax
                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token2);

                criteriaString = RtParseStringAllocAndCopy(
                    originalString + currentPosition);

                if (*criteriaString != '\0')
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "node %u: Wrong syntax for REMARK "
                        "access list in router config file.\n%s\n",
                        node->nodeId, originalString);
                    ERROR_ReportError(errString);
                }

                MEM_free(criteriaString);
                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration
                && (RtParseStringNoCaseCompare("IP", token1) == 0)
                && (RtParseStringNoCaseCompare(
                        "REFLEXIVE-LIST", token2) == 0)
                && (RtParseStringNoCaseCompare("TIMEOUT", token3) == 0))
            {
                // handle timeout
                if (*token4 == '\0' || (isalpha((int)*token4)))
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "node %u: Wrong syntax for timeout "
                        "in router config file.\n%s\n", node->nodeId,
                        originalString);
                    ERROR_ReportError(errString);
                }

                if ((!IO_IsStringNonNegativeInteger(token4)) ||
                    (((clocktype)(atof(token4)) * SECOND) <
                             ACCESS_LIST_MIN_REFLEX_TIMEOUT) ||
                    (((clocktype)(atof(token4)) * SECOND) >
                             ACCESS_LIST_MAX_REFLEX_TIMEOUT))
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "node %u: Reflex list timeout"
                        " must be positive integer from 0 to 2147483S "
                        "in router config file.\n%s\n", node->nodeId,
                        originalString);
                    ERROR_ReportError(errString);
                }

                ip->reflexTimeout = (clocktype)(atof(token4)) * SECOND;

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration
                && (RtParseStringNoCaseCompare("IP", token1) == 0)
                && (RtParseStringNoCaseCompare(
                        "ACCESS-LIST", token2) == 0))
            {
                // this is for named access list
                int currentPosition;

                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token2);

                criteriaString = RtParseStringAllocAndCopy(
                    originalString + currentPosition);

                refCriteriaString = criteriaString;

                AccessListParseNameCriteria(node,
                                            originalString,
                                            ip,
                                            &refCriteriaString);

                MEM_free(criteriaString);
                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration
                && ((RtParseStringNoCaseCompare("DENY", token1) == 0)
                || (RtParseStringNoCaseCompare("PERMIT", token1) == 0)))
            {
                // for named access list...carryover frm next line.
                BOOL isStandard = FALSE;
                AccessList* accessList = NULL;
                AccessList* newAccessList = NULL;

                // we take that its the last named access list
                AccessListGetLastNameAccessList(ip,
                                                &accessList,
                                                &isStandard);

                if (accessList == NULL)
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "node %u: Wrong syntax for access "
                        "list in router config file.\n%s\n", node->nodeId,
                        originalString);
                    ERROR_ReportError(errString);
                }

                if (accessList->continueNextLine == FALSE)
                {
                    newAccessList = AccessListCreateTemplate();
                    accessList->next = newAccessList;

                    // update the values of this access list, which
                    //  we have not got
                    newAccessList->accessListName =
                        RtParseStringAllocAndCopy(
                            accessList->accessListName);
                    newAccessList->filterType = accessList->filterType;
                }
                else
                {
                    newAccessList = accessList;
                    accessList->continueNextLine = FALSE;
                }

                criteriaString = RtParseStringAllocAndCopy(
                    originalString);
                refCriteriaString = criteriaString;

                AccessListParseCriteria(node,
                                        originalString,
                                        ip,
                                        &refCriteriaString,
                                        newAccessList,
                                        isStandard);

                MEM_free(criteriaString);
                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (RtParseStringNoCaseCompare("INTERFACE", token1) == 0))
            {
                // Got one interface statement so following lines are
                // association of access list to this interface
                AccessListProcessInterfaceStatement(
                    node,
                    rtConfig,
                    count,
                    interfaceName,
                    &isIndex);

                if (isIndex)
                {
                    sscanf(rtConfig.inputStrings[count], "%*s %d",
                        &interfaceIndex);
                    if ((interfaceIndex < 0) ||
                        (interfaceIndex >= node->numberInterfaces))
                    {
                        char errString[MAX_STRING_LENGTH];
                        sprintf(errString, "node %u: Wrong Interface index"
                            " in router config file\n%s\n", node->nodeId,
                            originalString);
                        ERROR_ReportError(errString);
                    }
                }
                // Interface number less than 0 or more than the number of
                // interfaces the node has.

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (RtParseStringNoCaseCompare("IP", token1) == 0) &&
                (RtParseStringNoCaseCompare("ACCESS-GROUP", token2) == 0))
            {
                // Got assciation of this interface and one accesslist
                // code changed from "IP ACCESS-GROUP" to token 1 and
                //   token2. User config may have "IP   ACCESS-GROUP"
                AccessListProcessIpAccessGroupStatement(
                    node,
                    originalString,
                    ip,
                    rtConfig,
                    count,
                    interfaceIndex);

                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (RtParseStringNoCaseCompare("IP", token1) == 0) &&
                (RtParseStringNoCaseCompare("ADDRESS", token2) == 0))
            {
                // set a primary or secondary IP address for an interface
                int currentPosition = 0;

                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token2);

                criteriaString = RtParseStringAllocAndCopy(originalString
                    + currentPosition);

                refCriteriaString = criteriaString;

                AccessListSetAddressForInterface(node,
                                                 &refCriteriaString,
                                                 originalString,
                                                 &interfaceIndex);

                MEM_free(criteriaString);
                MEM_free(originalString);
                continue;
            }
            else if (foundRouterConfiguration &&
                (RtParseStringNoCaseCompare("REMARK", token1) == 0))
            {
                int currentPosition = 0;

                currentPosition = RtParseGetPositionInString(
                    rtConfig.inputStrings[count], token1);

                criteriaString = RtParseStringAllocAndCopy(
                    originalString + currentPosition);

                refCriteriaString = criteriaString;

                AccessListAssignRemarkToName(node,
                                             &refCriteriaString,
                                             originalString);

                MEM_free(originalString);
                continue;
            }

            MEM_free(originalString);
        }

        // Free all name value pairs before exit
        if (ip->allNameValueLists != NULL)
        {
            AccessListFreeNameValue(ip);
        }

        gotForThisNode = FALSE;
        AccessListTraceInit(node, nodeInput);
        AccessListStatInit(node, nodeInput);
    }
}




//--------------------------------------------------------------------------
// FUNCTION  AccessListFinalize
// PURPOSE   Access list functionalities handled during termination.
//              Here we print the various access list statistics.
// ARGUMENTS node, current node
// RETURN    None
//--------------------------------------------------------------------------
void AccessListFinalize(Node* node)
{
    int i = 0;
    char buf[MAX_STRING_LENGTH];
    AccessListStats* stat = NULL;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    // print interface-wise
    for (i = 0; i < node->numberInterfaces; i++)
    {
        stat = &(ip->interfaceInfo[i]->accessListStat);

        ERROR_Assert(stat != NULL, "Access list stats not initialized.\n");

        sprintf(buf, "Packet Dropped at IN  = %u",
            stat->packetDroppedByStdAtIN);
        IO_PrintStat(node, "Network", "Stnd-ACL",
            ip->interfaceInfo[i]->ipAddress, -1, buf);

        sprintf(buf, "Packet Dropped at OUT = %u",
            stat->packetDroppedByStdAtOut);
        IO_PrintStat(node, "Network", "Stnd-ACL",
            ip->interfaceInfo[i]->ipAddress, -1, buf);

        sprintf(buf, "Packet Dropped at IN  = %u",
            stat->packetDroppedByExtdAtIN);
        IO_PrintStat(node, "Network", "Extd-ACL",
            ip->interfaceInfo[i]->ipAddress, -1, buf);

        sprintf(buf, "Packet Dropped at OUT = %u",
            stat->packetDroppedByExtdAtOUT);
        IO_PrintStat(node, "Network", "Extd-ACL",
            ip->interfaceInfo[i]->ipAddress, -1, buf);

        sprintf(buf, "Packet Dropped for mismatch at IN  = %u",
            stat->packetDroppedForMismatchAtIN);
        IO_PrintStat(node, "Network", "ACL",
            ip->interfaceInfo[i]->ipAddress, -1, buf);

        sprintf(buf, "Packet Dropped for mismatch at OUT = %u",
            stat->packetDroppedForMismatchAtOut);
        IO_PrintStat(node, "Network", "ACL",
            ip->interfaceInfo[i]->ipAddress, -1, buf);
    }
}




//--------------------------------------------------------------------------
// FUNCTION  AccessListMatchPort
// PURPOSE   Check if port conditions are matching
// ARGUMENTS packetPortNumber, port number of packet
//           accessList, pointer to access list
//           isSrc, is source or destination port
//           isAMatch, return the match result
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListMatchPort(
    unsigned short packetPortNumber,
    AccessList* accessList,
    BOOL isSrc,
    AccessListTriStateType* isAMatch)
{

    unsigned short accessListPort = 0;
    AccessListOperatorType accessListOperator;
    if (isSrc == 1)
    {
        accessListPort = (unsigned short)
            accessList->tcpOrUdpParameters->srcPortNumber;

        accessListOperator = accessList->tcpOrUdpParameters->srcOperator;
    }
    else
    {
        accessListPort = (unsigned short)
            accessList->tcpOrUdpParameters->destPortNumber;
        accessListOperator = accessList->tcpOrUdpParameters->destOperator;
    }

    if (accessListPort == 0)
    {
        *isAMatch = ACCESS_LIST_NA;
        return;
    }

    *isAMatch = ACCESS_LIST_NO_MATCH;

    switch (accessListOperator)
    {
        case ACCESS_LIST_EQUALITY:
        {
            if (accessListPort == packetPortNumber)
            {
                *isAMatch = ACCESS_LIST_MATCH;
            }
            break;
        }
        case ACCESS_LIST_NOT_EQUAL:
        {
            if (accessListPort != packetPortNumber)
            {
                *isAMatch = ACCESS_LIST_MATCH;
            }
            break;
        }
        case ACCESS_LIST_GREATER_THAN:
        {
            if (accessListPort < packetPortNumber)
            {
                *isAMatch = ACCESS_LIST_MATCH;
            }
            break;
        }
        case ACCESS_LIST_LESS_THAN:
        {
            if (accessListPort > packetPortNumber)
            {
                *isAMatch = ACCESS_LIST_MATCH;
            }
            break;
        }
        case ACCESS_LIST_RANGE:
        {
            if ((accessList->tcpOrUdpParameters->initialPortNumber <=
                accessListOperator) && (accessList->tcpOrUdpParameters->
                finalPortNumber >= accessListOperator))
            {
                *isAMatch = ACCESS_LIST_MATCH;
            }
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "AccessListMatchPort: Unknown operator"
                "type.\n");
            break;
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListCheckAddress
// PURPOSE   Match the Address
// ARGUMENTS accessList, pointer to access list
//           packetAdd, packet address to match
//           isSrc, is source or destination address
//           isAddress, match result
// RETURN    None
//--------------------------------------------------------------------------
void AccessListCheckAddress(AccessList* accessList,
                            NodeAddress packetAdd,
                            BOOL isSrc,
                            AccessListTriStateType* isAddress)
{
    NodeAddress address;
    NodeAddress mask;

    if (isSrc)
    {
        mask = accessList->srcMaskAddr;
        // masking rule in CISCO access lists
        mask = ~mask;
        address = mask & (accessList->srcAddr);
    }
    else
    {
        mask = accessList->destMaskAddr;
        // masking rule in CISCO access lists
        mask = ~mask;
        address = mask & (accessList->destAddr);
    }

    if (!address)
    {
        *isAddress = ACCESS_LIST_NA;
        return;
    }

    if ((mask & packetAdd) == address)
    {
        *isAddress = ACCESS_LIST_MATCH;
    }
    else
    {
        *isAddress = ACCESS_LIST_NO_MATCH;
    }
}


//--------------------------------------------------------------------------
// FUNCTION  CheckTCPParamsTrueOrFalse
// PURPOSE   Check a msg for matching with the access list
// ARGUMENTS isSourceAddress, match result of source address
//           isDestAddress, match result of destination address
//           isTos, match result of TOS
//           isSrcPort, match result of Source port
//           isDestPort, match result of destination port
//           isEstd, match result of Established parameter
//           isAMatch, return reuslt of match of all sub params
// RETURN    None
//--------------------------------------------------------------------------
static
void CheckTCPParamsTrueOrFalse(AccessListTriStateType isSourceAddress,
                               AccessListTriStateType isDestAddress,
                               AccessListTriStateType isTos,
                               AccessListTriStateType isSrcPort,
                               AccessListTriStateType isDestPort,
                               AccessListTriStateType isEstd,
                               BOOL* isAMatch)
{
    if (((isSourceAddress & ACCESS_LIST_NO_MATCH) ||
        (isDestAddress & ACCESS_LIST_NO_MATCH) ||
        (isTos & ACCESS_LIST_NO_MATCH) ||
        (isSrcPort & ACCESS_LIST_NO_MATCH) ||
        (isDestPort & ACCESS_LIST_NO_MATCH) ||
        (isEstd & ACCESS_LIST_NO_MATCH)) == ACCESS_LIST_NO_MATCH)
    {
        *isAMatch = FALSE;
    }
    else
    {
        *isAMatch = TRUE;
    }
}


//--------------------------------------------------------------------------
// FUNCTION  CheckICMPParamsTrueOrFalse
// PURPOSE   Check a msg for matching with the access list
// ARGUMENTS isSourceAddress, match result of source address
//           isDestAddress, match result of destination address
//           isTos, match result of TOS
//           isICMPType, match result of ICMP type
//           isICMPCode, match result of ICMP code
//           isAMatch, return reuslt of match of all sub params
// RETURN    None
//--------------------------------------------------------------------------
static
void CheckICMPParamsTrueOrFalse(AccessListTriStateType isSourceAddress,
                                AccessListTriStateType isDestAddress,
                                AccessListTriStateType isTos,
                                AccessListTriStateType isICMPType,
                                AccessListTriStateType isICMPCode,
                                BOOL* isAMatch)
{
    if (((isSourceAddress & ACCESS_LIST_NO_MATCH) ||
        (isDestAddress & ACCESS_LIST_NO_MATCH) ||
        (isTos & ACCESS_LIST_NO_MATCH) ||
        (isICMPType & ACCESS_LIST_NO_MATCH) ||
        (isICMPCode & ACCESS_LIST_NO_MATCH)) == ACCESS_LIST_NO_MATCH)
    {
        *isAMatch = FALSE;
    }
    else
    {
        *isAMatch = TRUE;
    }
}


//--------------------------------------------------------------------------
// FUNCTION  CheckIGMPParamsTrueOrFalse
// PURPOSE   Check a msg for matching with the access list
// ARGUMENTS isSourceAddress, match result of source address
//           isDestAddress, match result of destination address
//           isTos, match result of TOS
//           isIGMPType, match result of IGMP type
//           isAMatch, return reuslt of match of all sub params
// RETURN    None
//--------------------------------------------------------------------------
static
void CheckIGMPParamsTrueOrFalse(AccessListTriStateType isSourceAddress,
                                AccessListTriStateType isDestAddress,
                                AccessListTriStateType isTos,
                                AccessListTriStateType isIGMPType,
                                BOOL* isAMatch)
{
    if (((isSourceAddress & ACCESS_LIST_NO_MATCH) ||
        (isDestAddress & ACCESS_LIST_NO_MATCH) ||
        (isTos & ACCESS_LIST_NO_MATCH) ||
        (isIGMPType & ACCESS_LIST_NO_MATCH)) == ACCESS_LIST_NO_MATCH)
    {
        *isAMatch = FALSE;
    }
    else
    {
        *isAMatch = TRUE;
    }
}

//--------------------------------------------------------------------------
// FUNCTION  AccesListCheckCommonParams
// PURPOSE   Check a msg for matching with the access list
// ARGUMENTS isSourceAddress, match result of source address
//           isDestAddress, match result of destination address
//           isTos, match result of TOS
//           isEstd, match result of established
//           isAMatch, return reuslt of match of all sub params
// RETURN    None
//--------------------------------------------------------------------------
static
void AccesListCheckCommonParams(AccessListTriStateType isSourceAddress,
                                AccessListTriStateType isDestAddress,
                                AccessListTriStateType isTos,
                                AccessListTriStateType isPrecedence,
                                AccessListTriStateType isEstd,
                                BOOL* isAMatch)
{
    if (((isSourceAddress & ACCESS_LIST_NO_MATCH) ||
        (isDestAddress & ACCESS_LIST_NO_MATCH) ||
        (isEstd & ACCESS_LIST_NO_MATCH) ||
        (isPrecedence & ACCESS_LIST_NO_MATCH) ||
        (isTos & ACCESS_LIST_NO_MATCH)) == ACCESS_LIST_NO_MATCH)
    {
        *isAMatch = FALSE;
    }
    else
    {
        *isAMatch = TRUE;
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListCheckTrueOrFalse
// PURPOSE   matching with the access list
// ARGUMENTS accessListVar, parameter field of access list
//           packetVar, parameter list of packet variable
//           isTrue, return result of match
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListCheckTrueOrFalse(int accessListVar,
                                int packetVar,
                                AccessListTriStateType* isTrue)
{
    if (accessListVar == -1)
    {
        *isTrue = ACCESS_LIST_NA;
    }
    else
    {
        if (packetVar == accessListVar)
        {
            *isTrue = ACCESS_LIST_MATCH;
        }
        else
        {
            *isTrue = ACCESS_LIST_NO_MATCH;
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListMatchMsg
// PURPOSE   Check a msg for matching with the access list
// ARGUMENTS accessList, pointer to access list
//           msg, Message packet to match
//           isAMatch, return result of match
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListMatchMsg(AccessList* accessList,
                        const Message* msg,
                        BOOL* isAMatch)
{
    // Check source address, dest address, protocol, source port,
    // destination port, tos, precdence,if it matches the current
    // access list

    AccessList packetInfo;
    char* pktPtr = NULL;
    unsigned int precedenceValue = 0;
    int ipHdrSize = 0;

    AccessListTriStateType isSourceAddress = ACCESS_LIST_NA;
    AccessListTriStateType isDestAddress = ACCESS_LIST_NA;
    AccessListTriStateType isTOS = ACCESS_LIST_NA;
    AccessListTriStateType isPrecedence = ACCESS_LIST_NA;

    AccessListTriStateType isSrcPort = ACCESS_LIST_NA;
    AccessListTriStateType isDestPort = ACCESS_LIST_NA;
    AccessListTriStateType isEstd = ACCESS_LIST_NA;

    AccessListTriStateType isICMPType = ACCESS_LIST_NA;
    AccessListTriStateType isICMPCode = ACCESS_LIST_NA;
    AccessListTriStateType isIGMPType = ACCESS_LIST_NA;

    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    ipHdrSize = IpHeaderSize(ipHeader);

    pktPtr = (char*)(MESSAGE_ReturnPacket(msg) + ipHdrSize);
    memset(&packetInfo, 0, sizeof(AccessList));

    packetInfo.srcAddr = ipHeader->ip_src;
    packetInfo.destAddr = ipHeader->ip_dst;

    packetInfo.protocol = (unsigned char)ipHeader->ip_p;
    packetInfo.tos = (signed char)IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len);
    precedenceValue = IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len) >> 5;
    packetInfo.precedence = (signed char)precedenceValue;

    // check protocol
    if ((accessList->protocol) &&
        (accessList->protocol != packetInfo.protocol))
    {
        *isAMatch = FALSE;
        return;
    }

    // src and dest address, tos same for all protocols
    // src address
    AccessListCheckAddress(accessList,
                           packetInfo.srcAddr,
                           TRUE,
                           &isSourceAddress);

    // we dont need these checks for standard acl
    if (accessList->isStandard == FALSE)
    {
        //destination address
        AccessListCheckAddress(accessList,
                               packetInfo.destAddr,
                               FALSE,
                               &isDestAddress);

        //tos
        AccessListCheckTrueOrFalse(accessList->tos, packetInfo.tos, &isTOS);

        //precedence
        AccessListCheckTrueOrFalse(accessList->precedence,
                                   packetInfo.precedence,
                                   &isPrecedence);
    }

    switch (accessList->protocol)
    {
        case IPPROTO_TCP:
        {
            struct tcphdr* tcpHeader = (struct tcphdr*) pktPtr;

            // established param
            if (accessList->established == FALSE)
            {
                isEstd = ACCESS_LIST_NA;
            }
            else if (accessList->established == TRUE)
            {
                if ((tcpHeader->th_flags & TH_RST) ||
                    (tcpHeader->th_flags & TH_ACK))
                {
                    isEstd = ACCESS_LIST_MATCH;
                }
                else
                {
                    isEstd = ACCESS_LIST_NO_MATCH;
                }
            }

            if (!accessList->tcpOrUdpParameters)
            {
                // check if true till now
                AccesListCheckCommonParams(isSourceAddress,
                                           isDestAddress,
                                           isTOS,
                                           isPrecedence,
                                           isEstd,
                                           isAMatch);
                return;
            }

            AccessListMatchPort(tcpHeader->th_sport,
                                accessList,
                                TRUE,
                                &isSrcPort);

            AccessListMatchPort(tcpHeader->th_dport,
                                accessList,
                                FALSE,
                                &isDestPort);

            CheckTCPParamsTrueOrFalse(isSourceAddress,
                                      isDestAddress,
                                      isTOS,
                                      isSrcPort,
                                      isDestPort,
                                      isEstd,
                                      isAMatch);
            break;
        }
        case IPPROTO_UDP:
        {
            TransportUdpHeader* udpHdr = (TransportUdpHeader*) pktPtr;

            if (!accessList->tcpOrUdpParameters)
            {
                // check if true till now
                AccesListCheckCommonParams(isSourceAddress,
                                           isDestAddress,
                                           isTOS,
                                           isPrecedence,
                                           isEstd,
                                           isAMatch);
                return;
            }

            AccessListMatchPort(udpHdr->sourcePort,
                                accessList,
                                TRUE,
                                &isSrcPort);

            AccessListMatchPort(udpHdr->destPort,
                                accessList,
                                FALSE,
                                &isDestPort);

            CheckTCPParamsTrueOrFalse(isSourceAddress,
                                      isDestAddress,
                                      isTOS,
                                      isSrcPort,
                                      isDestPort,
                                      isEstd,
                                      isAMatch);
            break;
        }
        case IPPROTO_ICMP:
        {
            IcmpHeader* icmpHeader = (IcmpHeader*) pktPtr;
            if (!accessList->icmpParameters)
            {
                AccesListCheckCommonParams(isSourceAddress,
                                           isDestAddress,
                                           isTOS,
                                           isPrecedence,
                                           isEstd,
                                           isAMatch);
                return;
            }

            AccessListCheckTrueOrFalse(accessList->icmpParameters->icmpType,
                                       icmpHeader->icmpMessageType,
                                       &isICMPType);

            AccessListCheckTrueOrFalse(accessList->icmpParameters->icmpCode,
                                       icmpHeader->icmpCode,
                                       &isICMPCode);

            CheckICMPParamsTrueOrFalse(isSourceAddress,
                                       isDestAddress,
                                       isTOS,
                                       isICMPType,
                                       isICMPCode,
                                       isAMatch);
            break;
        }
        case IPPROTO_IGMP:
        {
            IgmpMessage* igmpHeader = (IgmpMessage*) pktPtr;
            if (!accessList->igmpParameters)
            {
                AccesListCheckCommonParams(isSourceAddress,
                                           isDestAddress,
                                           isTOS,
                                           isPrecedence,
                                           isEstd,
                                           isAMatch);
                return;
            }

            // igmp type is defined
            if (accessList->igmpParameters->igmpType == igmpHeader->
                ver_type)
            {
                isIGMPType = ACCESS_LIST_MATCH;
            }
            else
            {
                isIGMPType = ACCESS_LIST_NO_MATCH;
            }

            CheckIGMPParamsTrueOrFalse(isSourceAddress,
                                       isDestAddress,
                                       isTOS,
                                       isIGMPType,
                                       isAMatch);
            break;
        }
        default:
        {
            AccesListCheckCommonParams(isSourceAddress,
                                       isDestAddress,
                                       isTOS,
                                       isPrecedence,
                                       isEstd,
                                       isAMatch);
            break;
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListPrintLog
// PURPOSE   Print the log
// ARGUMENTS accessList, access list pointer
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListPrintLog(AccessList* accessList)
{
    char srcAddress[MAX_STRING_LENGTH];
    char srcMask[MAX_STRING_LENGTH];
    char destAddress[MAX_STRING_LENGTH];
    char destMask[MAX_STRING_LENGTH];

    IO_ConvertIpAddressToString(accessList->srcAddr, srcAddress);
    IO_ConvertIpAddressToString(accessList->srcMaskAddr, srcMask);

    IO_ConvertIpAddressToString(accessList->destAddr, destAddress);
    IO_ConvertIpAddressToString(accessList->destMaskAddr, destMask);

    if (accessList->accessListName)
    {
        printf("\nAccess List name: %s", accessList->accessListName);
    }
    else
    {
        printf("\nAccess List number: %d", accessList->accessListNumber);
    }

    if (accessList->remark[0] != '\0')
    {
        printf("\nRemark: %s", accessList->remark);
    }

    printf("\nSource IP:           %s", srcAddress);
    printf("\nSource mask IP:      %s", srcMask);
    printf("\nDestination IP:      %s", destAddress);
    printf("\nDestination mask IP: %s", destMask);

    switch (accessList->protocol)
    {
        case IPPROTO_TCP:
        {
            printf("\nProtocol: TCP");
            break;
        }
        case IPPROTO_UDP:
        {
            printf("\nProtocol: UDP");
            break;
        }
        case IPPROTO_ICMP:
        {
            printf("\nProtocol: ICMP");
            break;
        }
        case IPPROTO_IGMP:
        {
            printf("\nProtocol: IGMP");
            break;
        }
        default:
        {
            break;
            // wont do anything
        }
    }

    if (accessList->tcpOrUdpParameters)
    {
        printf("\nSource port: %d", accessList->
            tcpOrUdpParameters->srcPortNumber);
        printf("\nDestination port: %d", accessList->
            tcpOrUdpParameters->destPortNumber);
    }

    if (accessList->filterType == 0)
    {
        printf("\nFilter type: Permit");
        printf("\nNumber of packets permitted: %d",
            accessList->packetCount);
    }
    else
    {
        printf("\nFilter type: Deny");
        printf("\nNumber of packets denied: %d",
            accessList->packetCount);
    }
    printf("\n-----------------------------------------------\n");
}



//--------------------------------------------------------------------------
// FUNCTION   AccessListFreeAccessList
// PURPOSE    Free the memory for this access list
// PARAMETERS accessList, access list to be removed
// RETURN     none
//--------------------------------------------------------------------------
static
void AccessListFreeAccessList(AccessList* accessList)
{
    if (accessList->accessListName)
    {
        MEM_free(accessList->accessListName);
    }

    if (accessList->tcpOrUdpParameters)
    {
        MEM_free(accessList->tcpOrUdpParameters);
    }

    if (accessList->reflexParams)
    {
        MEM_free(accessList->reflexParams);
    }

    accessList = NULL;
}



//--------------------------------------------------------------------------
// FUNCTION   AccessListUpdateCorrespondingSession
// PURPOSE    Update the session status of the corresponding session id,
// PARAMETERS ip, ip data structure
//            session, session whose corresponding needs to be updated
// RETURN     none
//--------------------------------------------------------------------------
static
void AccessListUpdateCorrespondingSession(NetworkDataIp* ip,
                                          AccessListSessionList* session)
{
    unsigned char protocol = session->protocol;
    NodeAddress destAddr = session->srcAddr;
    NodeAddress srcAddr = session->destAddr;
    int srcPort = session->destPortNumber;
    int destPort = session->srcPortNumber;
    AccessListSessionList* tempSession =  ip->accessListSession;

    while (tempSession)
    {
        if ((tempSession->destAddr == destAddr)
            && (tempSession->destPortNumber == destPort)
            && (tempSession->protocol == protocol)
            && (tempSession->srcAddr == srcAddr)
            && (tempSession->srcPortNumber == srcPort))
        {
            tempSession->isRemoved = TRUE;
            return;
        }
        else
        {
            tempSession = tempSession->next;
        }
    }
}



//--------------------------------------------------------------------------
// FUNCTION   AccessListUpdateSessionStatus
// PURPOSE    Update the session status
// PARAMETERS ip, ip data structure
//            id, session id associated with the access list
// RETURN     BOOL result of whether updated
//--------------------------------------------------------------------------
static
BOOL AccessListUpdateSessionStatus(NetworkDataIp* ip, int id)
{
    AccessListSessionList* session = ip->accessListSession;
    while (session)
    {
        if (session->id == id)
        {
            if (session->isRemoved == FALSE)
            {
                session->isRemoved = TRUE;

                // adjust the changes in the corresponding session
                AccessListUpdateCorrespondingSession(ip, session);

                return TRUE;
            }
            else
            {
                ERROR_Assert(FALSE, "Access List already removed.\n");
                return FALSE;
            }
        }
        session = session->next;
    }

    // not found
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION   AccessListIsAccessListRemoved
// PURPOSE    Update the session status
// PARAMETERS node, node concerned
//            id, session id associated with the access list
// RETURN     BOOL result of whether updated
//--------------------------------------------------------------------------
static
BOOL AccessListIsAccessListRemoved(Node* node, int id)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    AccessListSessionList* session = ip->accessListSession;
    while (session)
    {
        if (session->id == id)
        {
            return session->isRemoved;
        }
        session = session->next;
    }

    // not found
    return FALSE;
}


//--------------------------------------------------------------------------
// FUNCTION   AccessListRemoveFilter
// PURPOSE    Remove the dynamic access list
// PARAMETERS node - this node
//            dynamicAccessList, dynamic accesslist
//            id, session id associated with the access list
// RETURN     none
//--------------------------------------------------------------------------
static
void AccessListRemoveFilter(Node *node,
                            AccessList* dynamicAccessList,
                            int id)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    AccessListName* accessListName = ip->accessListName;
    AccessListNest* nestReflex = ip->nestReflex;
    AccessList* headAccessList = NULL;
    BOOL isMatch = FALSE;
    BOOL isFound = FALSE;
    BOOL isRemoved = FALSE;
    int position = 0;
    char* nameAccessList = NULL;
    char* dynamicName =
        RtParseStringAllocAndCopy(dynamicAccessList->accessListName);

    if (AccessListIsAccessListRemoved(node, id))
    {
        MEM_free(dynamicName);
        return;
    }

    ERROR_Assert(dynamicAccessList->isDynamicallyCreated == TRUE,
        "AccessList not dynamically created.\n");

    // get the name of name access list from the ip structure
    while (nestReflex != NULL)
    {
        if (RtParseStringNoCaseCompare(
                dynamicAccessList->accessListName,
                nestReflex->name) == 0)
        {
            isMatch = TRUE;
            break;
        }
        nestReflex = nestReflex->next;
    }

    ERROR_Assert(isMatch == TRUE, "No Corresponding access list "
        "definition found.\n");

    position = nestReflex->position;
    nameAccessList =
        RtParseStringAllocAndCopy(nestReflex->accessListName);

    // get the dynamic name
    while (accessListName != NULL)
    {
        if (RtParseStringNoCaseCompare(
                accessListName->name,
                nameAccessList) == 0)
        {
            isFound = TRUE;
            break;
        }
        accessListName = accessListName->next;
    }

    ERROR_Assert(isFound == TRUE, "No Corresponding reflex access list "
        "definition found.\n");

    // get the first access list from the name list
    headAccessList = accessListName->accessList;

    // check if head is altered
    if (headAccessList->isDynamicallyCreated
        && headAccessList->next
        && (!headAccessList->next->isDynamicallyCreated)
        && (position == 1))
    {
        // the position has to be 1 and the first access list is created
        // dynamically and the next one if existing, is not a reflex
        // then, alter the head of access list from the name list
        accessListName->accessList = headAccessList->next;
        isRemoved = TRUE;
    }
    else
    {
        AccessList* currentAccessList = headAccessList;
        AccessList* prevAccessList = NULL;

        while (currentAccessList)
        {
            // is reference pointer from message equal to any pointer
            // from the list of accesslists
            if (dynamicAccessList == currentAccessList)
            {
                // it should have the same position as well
                if (currentAccessList->reflexParams &&
                    (currentAccessList->reflexParams->position == position))
                {
                    if (prevAccessList == 0)
                    {
                        accessListName->accessList =
                            currentAccessList->next;
                    }
                    else
                    {
                        prevAccessList->next = currentAccessList->next;
                    }

                    isRemoved = TRUE;
                    break;
                }
                else
                {
                    ERROR_Assert(FALSE, "No corresponding"
                        "Reflex ACL to be removed.\n");
                }
            }
            else
            {
                prevAccessList = currentAccessList;
                currentAccessList = currentAccessList->next;
            }
        }
    }

    ERROR_Assert(isRemoved == TRUE, "No corresponding Reflex ACL to"
        " be removed.\n");

    AccessListFreeAccessList(dynamicAccessList);

    // after removal, lets mark that as removed
    if (!AccessListUpdateSessionStatus(ip, id))
    {
        ERROR_Assert(FALSE, "No sesion ID associated with this "
            "Access List.\n");
    }

    // for debugging
    if (ACCESS_LIST_REFLEX_DEBUG)
    {
        char srcAddr[MAX_STRING_LENGTH];
        char destAddr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(dynamicAccessList->srcAddr, srcAddr);
        IO_ConvertIpAddressToString(dynamicAccessList->destAddr, destAddr);
        printf("-------------------------------------------\n");
        printf("Node: %u\n", node->nodeId);
        printf("Reflex access list %s \n Source Address: %s\n Destination "
            "Address: %s\n removed at %15f ...\n", dynamicName,
            srcAddr, destAddr, node->getNodeTime()/((float)SECOND));
        printf("-------------------------------------------\n");
    }

    MEM_free(dynamicAccessList);
    dynamicAccessList = NULL;
    MEM_free(nameAccessList);
    MEM_free(dynamicName);
}


//--------------------------------------------------------------------------
// FUNCTION   AccessListRemoveFilterForTimeout
// PURPOSE    Remove the filter or set timer as per the flag of TCP packet
// PARAMETERS node, this node.
//            dynamicAccessList, access list to be removed
//            id, id associated with this access list
// RETURN     BOOL result of whether the timeout is true
//--------------------------------------------------------------------------
static
BOOL AccessListRemoveFilterForTimeout(Node* node,
                                      AccessList* dynamicAccessList,
                                      int id)
{
    // check whether the access list still remains
    if (AccessListIsAccessListRemoved(node, id))
    {
        return TRUE;
    }

    ERROR_Assert(dynamicAccessList->reflexParams != NULL,
        "Reflex access list not created.\n");

    // if the node has not recieved any packet for the timeout time, remove
    //  the filter
    if (node->getNodeTime()
        >= (dynamicAccessList->reflexParams->timeout +
        dynamicAccessList->reflexParams->lastPacketSent))
    {
        // time out, so remove the filter
        AccessListRemoveFilter(node, dynamicAccessList, id);
        return TRUE;
    }

    return FALSE;
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListMatchOldSessions
// PURPOSE   Check whether a new connection is initiated
// ARGUMENTS ip, network data pointer
//           msg, the packet recieved
//           id, session id associated to this access list
// RETURN    BOOL result of whether the conxn is new
//--------------------------------------------------------------------------
static
BOOL AccessListMatchOldSessions(NetworkDataIp* ip,
                                const Message* msg,
                                int* id)
{
    AccessListSessionList* sessionList = ip->accessListSession;

    // extract the packet
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    int ipHdrSize = IpHeaderSize(ipHeader);
    char* pktPtr = (char*)(MESSAGE_ReturnPacket(msg) + ipHdrSize);
    struct tcphdr* tcpHeader = (struct tcphdr*) pktPtr;

    // check type of protocol, src address,
    //  dest address, src port, dest port
    unsigned char protocol = (unsigned char)ipHeader->ip_p;

    NodeAddress srcAddr = ipHeader->ip_src;
    NodeAddress destAddr = ipHeader->ip_dst;

    int srcPort = tcpHeader->th_sport;
    int destPort = tcpHeader->th_dport;

    while (sessionList != NULL)
    {
        if ((sessionList->protocol == protocol)
            && (sessionList->srcAddr == srcAddr)
            && (sessionList->destAddr == destAddr)
            && (sessionList->srcPortNumber == srcPort)
            && (sessionList->destPortNumber == destPort))
        {
            *id = sessionList->id;
            return TRUE;
        }

        sessionList = sessionList->next;
    }
    return FALSE;
}



//--------------------------------------------------------------------------
// FUNCTION   AccessListRemoveFilterIfTCPConxnOver
// PURPOSE    Remove the filter or set timer as per the flag of TCP packet
// PARAMETERS node - this node.
//            dynamicAccessList, access list to be removed
//            msg, message pointer
//            id, session id associated with access list
// RETURN     none
//--------------------------------------------------------------------------
static
void AccessListRemoveFilterIfTCPConxnOver(Node* node,
                                          AccessList* dynamicAccessList,
                                          const Message* msg,
                                          int id)
{
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    int ipHdrSize = IpHeaderSize(ipHeader);
    char* pktPtr = (char*)(MESSAGE_ReturnPacket(msg) + ipHdrSize);
    struct tcphdr* tcpHeader = (struct tcphdr*) pktPtr;
    unsigned char protocol = (unsigned char)ipHeader->ip_p;

    if (protocol != IPPROTO_TCP)
    {
        // packet protocol not TCP, so no point in checking further
        return;
    }

    if (tcpHeader->th_flags & TH_RST)
    {
        // RST flag set, so drop immediately
        AccessListRemoveFilter(node, dynamicAccessList, id);
        return;
    }

    // check the TWO set FIN bits
    if (tcpHeader->th_flags & TH_FIN)
    {
        // FIN detected...so increment the count
        dynamicAccessList->reflexParams->countFIN++;

        if (dynamicAccessList->reflexParams->countFIN == 2)
        {
            // send a timer to remove the filter after 5 sec
            // ie, ACCESS_LIST_TCP_FIN_SET_REFLEX_TIMEOUT
            AccessListSetTimer(node,
                               MSG_NETWORK_AccessList,
                               ACCESS_LIST_REFLEX_TCP_TIMEOUT_TIMER,
                               ACCESS_LIST_TCP_FIN_SET_REFLEX_TIMEOUT,
                               dynamicAccessList,
                               id);
            if (ACCESS_LIST_REFLEX_DEBUG)
            {
                char srcAddr[MAX_STRING_LENGTH];
                char destAddr[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(
                    dynamicAccessList->srcAddr, srcAddr);
                IO_ConvertIpAddressToString(
                    dynamicAccessList->destAddr, destAddr);
                printf("-------------------------------------------\n");
                printf("Node: %u\n", node->nodeId);
                printf("Reflex access list %s \n Source Address: %s\n "
                    "Destination Address: %s\n Setting 5 sec window timer"
                    " for FIN at %15f ...\n",
                    dynamicAccessList->accessListName,
                    srcAddr, destAddr,
                    node->getNodeTime()/((float)SECOND));
                printf("-------------------------------------------\n");
            }
        }
        else
        {
            // set it true
            dynamicAccessList->reflexParams->countFIN++;
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION   AccessListSetTimeoutTimer
// PURPOSE    Set timer for removing the dynamic reflex list for
//             connectionless transfer
// PARAMETERS node - this node.
//            accessList, the dynamic access list
//            id, session id asociated with this access list
// RETURN     None
//--------------------------------------------------------------------------
static
void AccessListSetTimeoutTimer(Node* node,
                               AccessList* accessList,
                               int id)
{
    AccessListSetTimer(node,
                       MSG_NETWORK_AccessList,
                       ACCESS_LIST_REFLEX_TIMEOUT_TIMER,
                       accessList->reflexParams->timeout,
                       accessList,
                       id);
}



//--------------------------------------------------------------------------
// FUNCTION   AccessListHandleEvent
// PURPOSE    Handle the access list events
// PARAMETERS node - this node.
//            msg, message to receive or transmit
// RETURN     None
//--------------------------------------------------------------------------
void AccessListHandleEvent(Node* node, Message *msg)
{
    AccessListMessageInfo* info =
        (AccessListMessageInfo*) MESSAGE_ReturnInfo(msg);

    int id = info->sessionId;
    AccessList* accessList = info->accessList;

    switch (info->timerType)
    {
        case ACCESS_LIST_LOG_TIMER:
        {
            // 5 mins are up, so print the log and set the next timer
            if (accessList->isFreed == TRUE)
            {
                if (accessList->accessListName != NULL)
                {
                    printf("Access List '%s' removed. Aborting log...\n",
                        accessList->accessListName);
                }
                else
                {
                    printf("Access List '%d' removed. Aborting log...\n",
                        accessList->accessListNumber);
                }
                MESSAGE_Free(node, msg);
                return;
            }
            printf("\n-----------------------------------------------\n");
            printf("\nRouter: %u", node->nodeId);
            AccessListPrintLog(accessList);

            MESSAGE_Send(node, msg, ACCESS_LIST_LOG_INTERVAL);
            break;
        }
        case ACCESS_LIST_REFLEX_TIMEOUT_TIMER:
        {
            // if timeout, remove the filter
            if (AccessListRemoveFilterForTimeout(node,
                                                 accessList,
                                                 id))
            {
                MESSAGE_Free(node, msg);
            }
            else
            {
                // adjust and retransmit the timer
                clocktype timeout =
                    (accessList->reflexParams->lastPacketSent +
                    accessList->reflexParams->timeout) -
                    node->getNodeTime();

                MESSAGE_Send(node, msg, timeout);

                if (ACCESS_LIST_REFLEX_DEBUG)
                {
                    char srcAddr[MAX_STRING_LENGTH];
                    char destAddr[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(
                        accessList->srcAddr, srcAddr);
                    IO_ConvertIpAddressToString(
                        accessList->destAddr, destAddr);
                    printf("-------------------------------------------\n");
                    printf("Node: %u\n", node->nodeId);
                    printf("Reflex access list %s \n Source Address: %s\n "
                        "Destination Address: %s\n Exending timer at"
                        " %15f ...\n",
                        accessList->accessListName, srcAddr, destAddr,
                        node->getNodeTime()/((float)SECOND));
                    printf("-------------------------------------------\n");
                }
            }
            break;
        }
        case ACCESS_LIST_REFLEX_TCP_TIMEOUT_TIMER:
        {
            // remove reflex list for TCP packets set for delay of 5 secs
            AccessListRemoveFilter(node, accessList, id);
            MESSAGE_Free(node, msg);
            break;
        }
        default:
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nnode :%u\nInvalid message type.\n",
                node->nodeId);
            ERROR_ReportError(errString);
            break;
        }
    }
}



//--------------------------------------------------------------------------
// FUNCTION    AccessListPrintTrace
// PURPOSE     Print trace for the access list
// ARGUMENTS   node, node concerned
//             msg, message pointer
//             accessList, Access list pointer
//             interfaceType, type of interface IN | OUT
//             interfaceIndex, interface index
//             match, TRUE access list matched
// RETURN      None
//--------------------------------------------------------------------------
static
void AccessListPrintTrace(Node* node,
                          const Message* msg,
                          AccessList* accessList,
                          AccessListInterfaceType interfaceType,
                          int interfaceIndex,
                          BOOL match)
{
    static int count = 0;
    FILE *fp = NULL;
    char address[ACCESS_LIST_ADDRESS_STRING];
    float simTime = 0.0;
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    if (count == 0)
    {
        fp = fopen("AccessListTrace.txt", "w");
        ERROR_Assert(fp != NULL,
            "AccessListTrace.txt: file initial open error.\n");

        fprintf(fp, "\n\nACCESS LIST TRACE FILE\n");
        fprintf(fp, "(Packets dropped for access list)\n\n");
        fprintf(fp, "----------------------------------------------\n");
        fprintf(fp, "Abbreviations and format used in this file:\n");
        fprintf(fp, "1)   Node ID\n");
        fprintf(fp, "2)   Interface index\n");
        fprintf(fp, "3)   Interface type(IN | OUT)\n");
        fprintf(fp, "4)   Time(sec)\n");
        fprintf(fp, "5)   Packet source address\n");
        fprintf(fp, "6)   Packet destination address\n");
        fprintf(fp, "7)   D: Deny filter  M: Access list mismatch\n");
        fprintf(fp, "8)   Access list protocol (NA: Not available)\n");
        fprintf(fp, "9)   Access List source address \n");
        fprintf(fp, "10)  Access List source mask address \n");
        fprintf(fp, "11)  Access List destination address\n");
        fprintf(fp, "12)  Access List destination mask address\n");
        fprintf(fp, "----------------------------------------------\n");
        fprintf(fp, "\n");
        fprintf(fp, "\n");
        fprintf(fp, "\n");
    }
    else
    {
        fp = fopen("AccessListTrace.txt", "a");
        ERROR_Assert(fp != NULL,
            "AccessListTrace.txt: file open error.\n");
    }

    count++;

    fprintf(fp, "\n%u ", node->nodeId);
    fprintf(fp, "%d ", interfaceIndex);

    if (interfaceType == ACCESS_LIST_IN)
    {
        fprintf(fp, "IN  ");
    }
    else
    if (interfaceType == ACCESS_LIST_OUT)
    {
        fprintf(fp, "OUT ");
    }

    simTime = node->getNodeTime() / ((float) SECOND);

    fprintf(fp, "%15f ", simTime);

    IO_ConvertIpAddressToString(ipHeader->ip_src, address);
    fprintf(fp, "%15s ", address);

    IO_ConvertIpAddressToString(ipHeader->ip_dst, address);
    fprintf(fp, "%15s ", address);

    if (match)
    {
        fprintf(fp, "  D  ");

        switch (accessList->protocol)
        {
            case IPPROTO_ICMP:
            {
                fprintf(fp, "ICMP");
                break;
            }
            case IPPROTO_IGMP:
            {
                fprintf(fp, "IGMP");
                break;
            }
            case IPPROTO_IP:
            {
                fprintf(fp, "IP  ");
                break;
            }
            case IPPROTO_TCP:
            {
                fprintf(fp, "TCP ");
                break;
            }
            case IPPROTO_UDP:
            {
                fprintf(fp, "UDP ");
                break;
            }
            default:
            {
                fprintf(fp, "NA  ");
                break;
            }
        }

        IO_ConvertIpAddressToString(accessList->srcAddr, address);
        fprintf(fp, "%15s ", address);

        IO_ConvertIpAddressToString(accessList->srcMaskAddr, address);
        fprintf(fp, "%15s ", address);

        IO_ConvertIpAddressToString(accessList->destAddr, address);
        fprintf(fp, "%15s ", address);

        IO_ConvertIpAddressToString(accessList->destMaskAddr, address);
        fprintf(fp, "%15s ", address);
    }
    else
    {
        fprintf(fp, "  M  ");
    }

    fclose(fp);
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListIsNewConxn
// PURPOSE   Check whether a new connection is initiated
// ARGUMENTS node, The node initializing
//           msg, message recieved
//           id, session id associated
// RETURN    BOOL result of whether the conx is new
//--------------------------------------------------------------------------
static
BOOL AccessListIsNewConxn(Node* node, const Message* msg, int* id)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    // extract the packet
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    int ipHdrSize = IpHeaderSize(ipHeader);

    // check type of protocol, src address,
    //  dest address, src port, dest port
    unsigned char protocol = (unsigned char)ipHeader->ip_p;

    NodeAddress srcAddr = ipHeader->ip_src;
    NodeAddress destAddr = ipHeader->ip_dst;

    char* pktPtr = (char*)(MESSAGE_ReturnPacket(msg) + ipHdrSize);
    struct tcphdr* tcpHeader = (struct tcphdr*) pktPtr;
    int srcPort = tcpHeader->th_sport;
    int destPort = tcpHeader->th_dport;

    //if ((protocol == IPPROTO_IGMP)
    //    || (protocol == IPPROTO_IGMP))
    //{
    //    return TRUE;
    //}

    // if session or transport already existing
    if (ip->accessListSession == NULL)
    {
        // intialize the list
        AccessListSessionList* newSession =
            (AccessListSessionList*)
            RtParseMallocAndSet(sizeof(AccessListSessionList));

        ip->accessListSession = newSession;
        newSession->id = 1;
        newSession->protocol = protocol;
        newSession->srcAddr = srcAddr;
        newSession->destAddr = destAddr;
        newSession->srcPortNumber = srcPort;
        newSession->destPortNumber = destPort;
        newSession->next = NULL;
        newSession->isRemoved = FALSE;

        *id = newSession->id;
        return TRUE;
    }
    else
    {
        BOOL match = FALSE;
        AccessListSessionList* sessionList = ip->accessListSession;

        match = AccessListMatchOldSessions(ip, msg, id);

        if (match == TRUE)
        {
            return FALSE;
        }
        else
        {
            // intialize the list
            AccessListSessionList* prevSession = NULL;
            AccessListSessionList* newSession =
                (AccessListSessionList*)
                RtParseMallocAndSet(sizeof(AccessListSessionList));

            // append this conxn info in the end
            while (sessionList->next != NULL)
            {
                prevSession = sessionList;
                sessionList = sessionList->next;
            }

            sessionList->next = newSession;

            newSession->id = sessionList->id + 1;
            newSession->protocol = protocol;
            newSession->srcAddr = srcAddr;
            newSession->destAddr = destAddr;
            newSession->srcPortNumber = srcPort;
            newSession->destPortNumber = destPort;
            newSession->isRemoved = FALSE;
            newSession->next = NULL;

            *id = newSession->id;
            return TRUE;
        }
    }
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListIsPacketFromMyNetwork
// PURPOSE   To check whether the packet belongs to my network
// ARGUMENTS node, The node initializing
//           interfaceIndex, index of the interface
//           srcAdd, packets incoming address
// RETURN    BOOL result of whether the packet is from the same network
//--------------------------------------------------------------------------
static
BOOL AccessListIsPacketFromMyNetwork(Node* node,
                                     NodeAddress srcAdd)
{
    int i = 0;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        NodeAddress subnetMask = NetworkIpGetInterfaceSubnetMask(node, i);
        NodeAddress networkAddress =
            MaskIpAddress(NetworkIpGetInterfaceAddress(node, i),
                          subnetMask);
        NodeAddress srcNetworkAddress =
            MaskIpAddress(srcAdd, subnetMask);

        if (networkAddress == srcNetworkAddress)
        {
            return TRUE;
        }
    }

    return FALSE;
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListCheckAndUpdateReflexForSetFIN
// PURPOSE   Check a packet whether FIN is set. If FIN is set, update the
//              corresponding reflex access list.
// ARGUMENTS node, The node initializing
//           msg. message recieved
//           accessList, corresponding to nested
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListCheckAndUpdateReflexForSetFIN(Node* node,
                                             const Message* msg,
                                             AccessList* accessList)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);
    int ipHdrSize = IpHeaderSize(ipHeader);
    char* pktPtr = (char*)(MESSAGE_ReturnPacket(msg) + ipHdrSize);
    struct tcphdr* tcpHeader = (struct tcphdr*) pktPtr;

    // if its got a set FIN
    if (tcpHeader->th_flags & TH_FIN)
    {
        AccessListReflex* reflexParam = NULL;
        BOOL isMatch = FALSE;
        BOOL isFound = FALSE;
        AccessListName* accessListName = NULL;
        AccessList* reflexAccessList = NULL;
        AccessListNest* nestReflex = NULL;

        reflexParam = accessList->reflexParams;
        ERROR_Assert(reflexParam != NULL, "No reflex access list defined."
            "\n");

        // get the named access list

        nestReflex = ip->nestReflex;

        while (nestReflex != NULL)
        {
            if (RtParseStringNoCaseCompare(nestReflex->name,
                reflexParam->name) == 0)
            {
                isFound = TRUE;
                break;
            }
            nestReflex = nestReflex->next;
        }

        if (isFound == FALSE)
        {
            return;
        }

        accessListName = ip->accessListName;
        while (accessListName != NULL)
        {
            if (RtParseStringNoCaseCompare(accessListName->name,
                                              nestReflex->accessListName)
                                              == 0)
            {
                isMatch = TRUE;
                break;
            }
            accessListName = accessListName->next;
        }

        if (isMatch == TRUE)
        {
            reflexAccessList = accessListName->accessList;

            // extract the access list
            while (reflexAccessList != NULL)
            {
                if (RtParseStringNoCaseCompare(
                    reflexAccessList->accessListName, reflexParam->name)
                    == 0)
                {
                    ERROR_Assert(
                        reflexAccessList->isDynamicallyCreated == TRUE,
                        "\nReflex access list not enabled.\n");

                    reflexAccessList->reflexParams->countFIN++;

                    return;
                }

                reflexAccessList = reflexAccessList->next;
            }
        }
    }
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListMatchPacket
// PURPOSE   Check a packet for matching with the access list
// ARGUMENTS node, The node initializing
//           accessListPointer, Pointer to Access List
//           msg. message recieved
//           interfaceType, which type of interface,
//           interfaceIndex, index of the interface
//           isAMatch, matching result
//           filter, type of filter ie permit or deny
//           isStd, is it a standard acl
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListMatchPacket(Node* node,
                           AccessListPointer* accessListPointer,
                           const Message* msg,
                           AccessListInterfaceType interfaceType,
                           int interfaceIndex,
                           BOOL* isAMatch,
                           AccessListFilterType* filter,
                           BOOL* isStd)
{
    AccessList* accessList = NULL;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    *isAMatch = FALSE;
    *filter = ACCESS_LIST_DENY;

    if (accessListPointer == NULL)
    {
        char errString[MAX_STRING_LENGTH];
        *isAMatch = TRUE;
        *filter = ACCESS_LIST_PERMIT;
        sprintf(errString, "Node: %u\n, Filtering aborted...\n",
            node->nodeId);
        ERROR_ReportWarning(errString);
        return;
    }

    // Check all the access lists associated with this interface.
    // Check one by one if there is a match.
    // If it is a match check if the packet is permitted or denied

    while (accessListPointer != NULL)
    {
        accessList = *(accessListPointer->accessList);
        while ((accessList) != NULL)
        {
            IpHeaderType* ipHeader = (IpHeaderType*)
                MESSAGE_ReturnPacket(msg);

            // if this is a proxy, for handling exceptional cases where
            //  just the name of access list is defined.
            if (accessList->isProxy)
            {
                if (accessListPointer->next == NULL)
                {
                    *isAMatch = TRUE;
                    *filter = ACCESS_LIST_PERMIT;
                    return;
                }
                else
                {
                    break;
                }
            }

            AccessListMatchMsg(accessList, msg, isAMatch);

            if (*isAMatch)
            {
                int id = 0;

                // assign filter
                *filter = accessList->filterType;
                *isStd = accessList->isStandard;

                // REFLEX functionalities follow

                // check the sesssion, such that no repetation of reflex
                //  takes place
                if (AccessListIsNewConxn(node, msg, &id) == TRUE)
                {
                    // check for reflexive definition, and whether the
                    //  packet is from my network, and the reflex
                    //  definition is triggered.
                    if ((AccessListIsPacketFromMyNetwork(node,
                        ipHeader->ip_src))
                        && (accessList->isReflectDefined == TRUE))
                    {
                        AccessList* reflexAccessList = NULL;
                        // reflexive access list triggered
                        // lets create the dynamic access list
                        reflexAccessList = AccessListCreateReflex(
                            node,
                            accessList,
                            msg);

                        if (ACCESS_LIST_REFLEX_DEBUG)
                        {
                            char srcAddr[MAX_STRING_LENGTH];
                            char destAddr[MAX_STRING_LENGTH];

                            IO_ConvertIpAddressToString(
                                reflexAccessList->srcAddr, srcAddr);
                            IO_ConvertIpAddressToString(
                                reflexAccessList->destAddr, destAddr);
                            printf("----------------------------------"
                                "---------\n");
                            printf("Node: %u\n", node->nodeId);
                            printf("Reflex access list %s \n Source "
                                "Address: %s\n Destination Address: %s"
                                "\nCreated at %15f ...\n",
                                reflexAccessList->accessListName,
                                srcAddr, destAddr,
                                node->getNodeTime()/((float)SECOND));
                            printf("-----------------------------------"
                                "--------\n");
                        }

                        // set the timeout timer
                        if (reflexAccessList)
                        {
                            reflexAccessList->reflexParams->sessionId = id;

                            AccessListSetTimeoutTimer(node,
                                                      reflexAccessList,
                                                      id);

                            // used for 0s timeout timer
                            if (AccessListIsAccessListRemoved(node, id))
                            {
                                return;
                            }

                            // for debugging
                            if (ACCESS_LIST_REFLEX_DEBUG)
                            {
                                char srcAddr[MAX_STRING_LENGTH];
                                char destAddr[MAX_STRING_LENGTH];

                                IO_ConvertIpAddressToString(
                                    reflexAccessList->srcAddr, srcAddr);
                                IO_ConvertIpAddressToString(
                                    reflexAccessList->destAddr, destAddr);
                                printf("----------------------------------"
                                    "---------\n");
                                printf("Node: %u\n", node->nodeId);
                                printf("Reflex access list %s \n Source "
                                    "Address: %s\n Destination Address: %s"
                                    "\nReleasing"
                                    " timeout timer of %15f ...\n",
                                    reflexAccessList->accessListName,
                                    srcAddr, destAddr,
                                    reflexAccessList->reflexParams
                                        ->timeout / ((float)SECOND));
                                printf("-----------------------------------"
                                    "--------\n");
                            }
                        }
                    }
                }
                else
                {
                    // actions follow for nested reflex access list

                    if (accessList->isDynamicallyCreated)
                    {
                         // we store the last packet sent time for removing
                        //  filter
                        accessList->reflexParams->lastPacketSent =
                            node->getNodeTime();

                        // check whether FIN or RST flg set for TCP
                        AccessListRemoveFilterIfTCPConxnOver(node,
                                                             accessList,
                                                             msg,
                                                             id);
                    }
                    else
                    {
                        // all we are doing is just to trap the FIN
                        //  for our reflex
                        if (accessList->isReflectDefined == TRUE)
                        {
                            // we have to check all the FIN's for reflex
                            //  removal
                            AccessListCheckAndUpdateReflexForSetFIN(node,
                                msg,
                                accessList);
                        }
                    }
                }

                // handling trace functions

                // specific actions taken for access list when a
                //  match has occured

                if ((ip->accessListTrace == TRUE)
                    && (accessList->filterType == ACCESS_LIST_DENY))
                {
                    AccessListPrintTrace(node, msg, accessList,
                       interfaceType, interfaceIndex, TRUE);
                }

                // handle logging
                if (accessList->logType == ACCESS_LIST_NO_LOG)
                {
                    return; // no log declared
                }

                if (accessList->logOn == FALSE)
                {
                    printf("\n--------------------------------------"
                        "---------\n");
                    printf("\nRouter: %u", node->nodeId);
                    accessList->packetCount++;
                    AccessListPrintLog(accessList);

                    // start a timer for the next log
                    AccessListSetTimer(node,
                                       MSG_NETWORK_AccessList,
                                       ACCESS_LIST_LOG_TIMER,
                                       ACCESS_LIST_LOG_INTERVAL,
                                       accessList,
                                       0);
                    accessList->logOn = TRUE;
                }
                else
                {
                    accessList->packetCount++;
                }
                return;
            }
            else
            {
                // not a match, so check next access list
                accessList = accessList->next;

                // if the next ACL is proxy, then, we break out
                if (accessList && accessList->isProxy)
                {
                    break;
                }
            }
        }

        // jump to next access list pointer
        accessListPointer = accessListPointer->next;
    }
}



//--------------------------------------------------------------------------
// FUNCTION  AccessListUpdateStats
// PURPOSE   Update th statistics when the packet has dropped
// ARGUMENTS ip, network data pointer
//           interfaceIndex, interface ID
//           interfaceType, type of interface ie IN or OUT
//           isMatch, is the access list matched
//           isStd, is it a standard or an extended ACL
// RETURN    none
//--------------------------------------------------------------------------
static
void AccessListUpdateStats(NetworkDataIp* ip,
                           int interfaceIndex,
                           AccessListInterfaceType interfaceType,
                           BOOL isMatch,
                           BOOL isStd)
{
    AccessListStats* stat =
        &(ip->interfaceInfo[interfaceIndex]->accessListStat);

    if (interfaceType == ACCESS_LIST_IN)
    {
        if (!isMatch)
        {
            stat->packetDroppedForMismatchAtIN++;
        }
        else
        {
            if (isStd)
            {
                stat->packetDroppedByStdAtIN++;
            }
            else
            {
                stat->packetDroppedByExtdAtIN++;
            }
        }
    }
    else
    {
        if (!isMatch)
        {
            stat->packetDroppedForMismatchAtOut++;
        }
        else
        {
            if (isStd)
            {
                stat->packetDroppedByStdAtOut++;
            }
            else
            {
                stat->packetDroppedByExtdAtOUT++;
            }
        }
    }
}




//--------------------------------------------------------------------------
// FUNCTION  AccessListFilterPacket
// PURPOSE   Filter a packet on the basis of its match with the access list
// ARGUMENTS node, The node initializing
//           msg, packet pointer
//           accessListPointer, pointer to access list pointer
//           interfaceIndex, interface ID
//           interfaceType, type of interface, for trace
// RETURN    BOOL, returns result of whether we should drop the packet. TRUE
//              for dropping.
//--------------------------------------------------------------------------
BOOL AccessListFilterPacket(Node* node,
                            Message* msg,
                            AccessListPointer* accessListPointer,
                            int interfaceIndex,
                            AccessListInterfaceType interfaceType)
{
    AccessListFilterType filter = ACCESS_LIST_DENY;
    BOOL isAMatch = FALSE;
    BOOL isStd = FALSE;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    IpHeaderType* ipHeader = (IpHeaderType*) msg->packet;

    AccessListMatchPacket(
        node,
        accessListPointer,
        msg,
        interfaceType,
        interfaceIndex,
        &isAMatch,
        &filter,
        &isStd);

    if (NetworkIpIsMyIP(node, ipHeader->ip_src))
    {
        return FALSE;
    }

    if ((isAMatch  ==  FALSE) || (filter == ACCESS_LIST_DENY))
    {
        if ((ip->accessListTrace == TRUE) &&
            (isAMatch == FALSE))
        {
            AccessListPrintTrace(node, msg, NULL,
                interfaceType, interfaceIndex, FALSE);
        }

        // Drop the packet
        MESSAGE_Free(node, msg);

        if (interfaceType == ACCESS_LIST_OUT)
        {
            AccessListUpdateStats(
                ip,
                interfaceIndex,
                ACCESS_LIST_OUT,
                isAMatch,
                isStd);
        }
        else if (interfaceType == ACCESS_LIST_IN)
        {
            AccessListUpdateStats(
                ip,
                interfaceIndex,
                ACCESS_LIST_IN,
                isAMatch,
                isStd);
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



//--------------------------------------------------------------------------
//------------------  CISCO FUNCTIONS -----------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// FUNCTION  AccessListPrintForGivenId
// PURPOSE   print the access list
// ARGUMENTS accessList, the accessList pointer
//           accessListId, the ID of the corresponding access list
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListPrintForGivenId(Node* node,
                               AccessList* accessList,
                               char* accessListId)
{
    char address[ACCESS_LIST_ADDRESS_STRING];
    char address1[ACCESS_LIST_ADDRESS_STRING];
    char mask[ACCESS_LIST_ADDRESS_STRING];
    char mask1[ACCESS_LIST_ADDRESS_STRING];


    if (accessList->isFreed == TRUE)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\n, "
            "AccessList: %s is freed. Display aborted...\n",
            node->nodeId, accessListId);
        ERROR_ReportWarning(errString);
        return;
    }

    printf("\n------------------------------------------------------");

    // if a proxy is created for reflex
    if (accessList->isProxy)
    {
        return;
    }

    if (accessListId)
    {
        printf("\nShowing access List %s ...", accessListId);
    }
    else
    {
        printf("\nShowing Access List %d ...",
            accessList->accessListNumber);
    }

    printf("\n..........................................\n");
    while (accessList != NULL)
    {
        if (accessList->isDynamicallyCreated)
        {
            printf("\n\nReflex entry...\n");
        }


        if (accessList->isProxy)
        {
            break;
        }

        IO_ConvertIpAddressToString(accessList->srcAddr, address);
        IO_ConvertIpAddressToString(accessList->srcMaskAddr, mask);
        IO_ConvertIpAddressToString(accessList->destAddr, address1);
        IO_ConvertIpAddressToString(accessList->destMaskAddr, mask1);

        if (accessList->remark[0] != '\0')
        {
            printf("\nRemark: %s", accessList->remark);
        }

        if (accessList->filterType == 0)
        {
            printf("\nFilter: PERMIT");
        }
        else
        {
            printf("\nFilter: DENY");
        }

        switch (accessList->protocol)
        {
            case IPPROTO_ICMP:
            {
                printf("\nProtocol: ICMP");
                break;
            }
            case IPPROTO_IGMP:
            {
                printf("\nProtocol: IGMP");
                break;
            }
            case IPPROTO_IP:
            {
                printf("\nProtocol: IP");
                break;
            }
            case IPPROTO_TCP:
            {
                printf("\nProtocol: TCP");
                break;
            }
            case IPPROTO_UDP:
            {
                printf("\nProtocol: UDP");
                break;
            }
            default:
            {
                //printf("\nNA");
                break;
            }
        }

        printf("\nSource IP Address: %s",address);
        printf("\nSource wild mask : %s",mask);
        printf("\nDestination IP Address: %s",address1);
        printf("\nDestination wild mask : %s",mask1);

        if (accessList->precedence != -1)
        {
            printf("\nPrecedence: %d",accessList->precedence);
        }
        if (accessList->tos != -1)
        {
            printf("\nTOS: %d",accessList->tos);
        }

        if (accessList->established == TRUE)
        {
            printf("\nEstablished Parameter set");
        }
        else
        {
            printf("\nEstablished Parameter not set");
        }
        if (accessList->logType == ACCESS_LIST_NO_LOG)
        {
            printf("\nNo logging feature enabled");
        }
        else
        {
            printf("\nLogging feature enabled");
        }

        if (accessList->tcpOrUdpParameters)
        {
            printf("\nSource Port Number: %d",accessList->
                tcpOrUdpParameters->srcPortNumber);
            printf("\nDestination Port Number: %d",accessList->
                tcpOrUdpParameters->destPortNumber);
        }

        if (accessList->icmpParameters)
        {
            printf("\nICMP Type: %d",accessList->icmpParameters->icmpType);
            printf("\nICMP Code: %d",accessList->icmpParameters->icmpCode);
        }

        if (accessList->igmpParameters)
        {
            printf("\nIGMP Type: %d",accessList->igmpParameters->igmpType);
        }

        printf("\n..........................................\n");
        accessList = accessList->next;
    }
    printf("\nEnd");
    printf("\n------------------------------------------------------\n");
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListSearchACL
// PURPOSE   Search a packet for matching with the access list
// ARGUMENTS node, node concerned
//           accessListId, name or number charactersing the Access List
// RETURN    Pointer to AccessList
//--------------------------------------------------------------------------
static
AccessList* AccessListSearchACL(Node* node, char* accessListId)
{
    AccessListName* nameAccessList = NULL;
    AccessList* accessList = NULL;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    if ((ip->accessList == NULL) && (ip->accessListName == NULL))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\n, No access list initialized."
            " Aborting ...\n", node->nodeId);
        ERROR_ReportError(errString);
        return NULL;
    }

    if (isdigit((int)*accessListId))
    {
        if ((atoi(accessListId) < 1) || (atoi(accessListId) > 199))
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "\nnode :%u\nAccess List should be between 1"
                " and 199.\n", node->nodeId);
            ERROR_ReportError(errString);
        }

        accessList = ip->accessList[atoi(accessListId) - 1];
        return accessList;
    }
    else if (isalpha((int)*accessListId))
    {
        nameAccessList = ip->accessListName;
        while (nameAccessList != NULL)
        {
            if (RtParseStringNoCaseCompare(nameAccessList->name,
                accessListId) == 0)
            {
                return (nameAccessList->accessList);
            }
            nameAccessList = nameAccessList->next;
        }
    }
    else
    {
        // this is junk
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\nWrong access list ID in "
            "'AccessListShowACL'.\n", node->nodeId);
        ERROR_ReportError(errString);
    }
    return NULL;
}


//--------------------------------------------------------------------------
// FUNCTION    AccessListShowACL
// PURPOSE     Show the access list (IOS function)
// ASSUMPTIONS Till we dont have a defined use of AccessListShowACL, its
//              taken for granted the ID is a string.
// ARGUMENTS   node, node concerned
//             accessListId, name or number charactersing the Access List
// RETURN      None
//--------------------------------------------------------------------------
void AccessListShowACL(Node* node, char* accessListId)
{
    AccessList* accessList = NULL;
    // using the default case, showing all
    if (accessListId == NULL)
    {
        AccessListName* nameAccessList = NULL;
        NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
        int i = 0;
        // for number access list
        printf("\nShowing All access lists...\n");

        if ((ip->accessList == NULL) && (ip->accessListName == NULL))
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "Node: %u\n'AccessListShowACL', "
                "No access list initialized. Aborting display...\n",
                node->nodeId);
            ERROR_ReportWarning(errString);
            return;
        }

        for (i = 0; i < ACCESS_LIST_MAX_NUM; i++)
        {
            if (ip->accessList[i])
            {
                accessList = ip->accessList[i];
                AccessListPrintForGivenId(node, accessList, NULL);
            }
        }

        // for name access list
        nameAccessList = ip->accessListName;
        while (nameAccessList != NULL)
        {
            AccessListPrintForGivenId(node,
                                      nameAccessList->accessList,
                                      nameAccessList->name);

            nameAccessList = nameAccessList->next;
        }

        return;
    }
    else
    {
        accessList = AccessListSearchACL(node, accessListId);

        if (accessList == NULL)
        {
            // no such entry found
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "node :%u\n'AccessListShowACL', Access list "
                "number not initialized.\n", node->nodeId);
            ERROR_ReportWarning(errString);
        }
        else
        {
            AccessListPrintForGivenId(node, accessList, accessListId);
        }
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListClearCounters
// PURPOSE   Check a packet for matching with the access list
// ASSUMPTIONS Till we dont have a defined use of AccessListShowACL, its
//              taken for granted the ID is a string.
// ARGUMENTS node, node concerned
//           accessListId, name or number charactersing the Access List
// RETURN    None
//--------------------------------------------------------------------------
void AccessListClearCounters(Node* node, char* accessListId)
{
    AccessList* accessList = NULL;

    accessList = AccessListSearchACL(node, accessListId);

    if (accessList == NULL)
    {
        // no such entry found
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "node :%u\n'AccessListClearCounters', Access "
            "list number not initialized.\n", node->nodeId);
        ERROR_ReportError(errString);
    }
    else
    {
        accessList->packetCount = 0;
    }
}


//--------------------------------------------------------------------------
// FUNCTION  AccessListFreeIfMatch
// PURPOSE   remove the access list link if a match occurs
// ARGUMENTS accessListPointer, pointer to head of the access list
//           accessListId, name or number charactersing the Access List
// RETURN    None
//--------------------------------------------------------------------------
static
void AccessListFreeIfMatch(Node* node,
                           AccessListPointer** accessListPointer,
                           char* accessListId)
{
    char errString[MAX_STRING_LENGTH];
    AccessList** accessList = NULL;
    AccessListPointer* tempAccessListPointer = NULL;
    AccessListPointer* prevAccessListPointer = NULL;
    tempAccessListPointer = *accessListPointer;

    while (tempAccessListPointer)
    {
        accessList = tempAccessListPointer->accessList;

        // check the AL number
        if (isdigit((int)*accessListId))
        {
            if ((*accessList)->accessListNumber ==
                atoi(accessListId))
            {
                // already freed
                if ((*accessList)->isFreed == TRUE)
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "Node: %u\n'AccessListNoACL', "
                        "AccessList: %d already removed...\n",
                        node->nodeId, (*accessList)->accessListNumber);
                    ERROR_ReportWarning(errString);
                    return;
                }
                // set the access list as is freed
                (*accessList)->isFreed = TRUE;

                // remove it from the list
                // is this the head
                if (tempAccessListPointer == *accessListPointer)
                {
                    *accessListPointer = (*accessListPointer)->next;
                }
                // redo the links of the list
                else
                {
                    prevAccessListPointer->next =
                        tempAccessListPointer->next;
                }
                printf("Removed AccessList: %d...\n",
                    (*accessList)->accessListNumber);
                return;
            }
        }
        // check the AL name
        else if (isalpha((int)*accessListId))
        {
            if (!strcmp((*accessList)->accessListName,
               accessListId))
            {
                // already freed
                if ((*accessList)->isFreed == TRUE)
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "Node: %u\n'AccessListNoACL', "
                        "AccessList: %s already removed...\n",
                        node->nodeId, (*accessList)->accessListName);
                    ERROR_ReportWarning(errString);
                    return;
                }

                // set the access list as is freed
                (*accessList)->isFreed = TRUE;

                // remove it from the list
                // is this the head
                if (tempAccessListPointer == *accessListPointer)
                {
                    *accessListPointer = (*accessListPointer)->next;
                }
                // redo the links of the list
                else
                {
                    prevAccessListPointer->next =
                        tempAccessListPointer->next;
                }
                printf("Removed AccessList: %s...\n",
                    (*accessList)->accessListName);
                return;
            }
        }
        else
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString, "Node: %u\n'AccessListNoACL', "
                "Junk access list ID. Aborting ...\n",
                node->nodeId);
            ERROR_ReportWarning(errString);
            return;
        }

        prevAccessListPointer = tempAccessListPointer;
        tempAccessListPointer = tempAccessListPointer->next;
    }

    sprintf(errString, "Node: %u\nAccessListNoACL: "
        "'%s' access list ID not initialized. Aborting ...\n",
        node->nodeId, accessListId);
    ERROR_ReportWarning(errString);
}


//--------------------------------------------------------------------------
// FUNCTION    AccessListNoACL
// PURPOSE     Remove the access list
// ASSUMPTIONS Till we dont have a defined use of AccessListNoACL, its
//              taken for granted the ID is a string.
// ARGUMENTS   node, node concerned
//             accessListId, name or number charactersing the Access List
// RETURN      None
//--------------------------------------------------------------------------
void AccessListNoACL(Node* node, char* accessListId)
{
    int i = 0;

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    if ((ip->interfaceInfo[i]->accessListInPointer == NULL) &&
        (ip->interfaceInfo[i]->accessListOutPointer == NULL))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "Node: %u\n'AccessListNoACL', "
            "No access list initialized. Aborting ...\n",
            node->nodeId);
        ERROR_ReportWarning(errString);
        return;
    }

    // removing from interface
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ip->interfaceInfo[i]->accessListInPointer)
        {
            AccessListFreeIfMatch(
                node,
                &(ip->interfaceInfo[i]->accessListInPointer),
                accessListId);
        }

        if (ip->interfaceInfo[i]->accessListOutPointer)
        {
            AccessListFreeIfMatch(
                node,
                &(ip->interfaceInfo[i]->accessListOutPointer),
                accessListId);
        }
    }
}




//--------------------------------------------------------------------------
// FUNCTION    AccessListVerifyRouteMap
// PURPOSE     Verify the match conditions of route map with the given
//              access list id (name / number)
// ARGUMENTS   node, node concerned
//             accessListId, name or number charactersing the Access List
//             destAdd, the destination address or network address to be
//                  matched
// RETURN      BOOL, the match result
//--------------------------------------------------------------------------
BOOL AccessListVerifyRouteMap(Node* node,
                              char* accessListId,
                              NodeAddress srcAddress,
                              NodeAddress destAddress)
{
    AccessList* accessList = AccessListSearchACL(node, accessListId);
    AccessListTriStateType isSrcAddress = ACCESS_LIST_NO_MATCH;
    AccessListTriStateType isDestAddress = ACCESS_LIST_NO_MATCH;

    if (!accessList)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nNo access list, '%s', defined."
            " Route map following default action...\n",
            node->nodeId, accessListId);
        ERROR_ReportWarning(errString);
        return FALSE;
    }

    if (accessList->isFreed)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "\nNode :%u\nNo access list, '%s', defined."
            " Route map following default action...\n",
            node->nodeId, accessListId);
        ERROR_ReportWarning(errString);
        return FALSE;
    }

    // we go for matching all the access lists in that ID, if
    while (accessList != NULL)
    {
        // if we get a untriggered reflex
        if (accessList->isProxy)
        {
            accessList = accessList->next;
        }

        // go for the match
        AccessListCheckAddress(accessList,
                               srcAddress,
                               TRUE,
                               &isSrcAddress);

        AccessListCheckAddress(accessList,
                               destAddress,
                               FALSE,
                               &isDestAddress);

        if (((isSrcAddress == ACCESS_LIST_MATCH) &&
            ((isDestAddress == ACCESS_LIST_MATCH) ||
            (isDestAddress == ACCESS_LIST_NA))) ||
            ((isDestAddress == ACCESS_LIST_MATCH) &&
            ((isSrcAddress == ACCESS_LIST_MATCH) ||
            (isSrcAddress == ACCESS_LIST_NA))))
        {
            return TRUE;
        }

        // traversing all the lists
        accessList = accessList->next;
    }

    if ((isSrcAddress == ACCESS_LIST_NA) &&
        (isDestAddress == ACCESS_LIST_NA))
    {
        return TRUE;
    }

    return FALSE;
}

