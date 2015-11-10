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
// use in compliance with the license agreement as part of the EXata
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include "api.h"
#include "app_util.h"
#include "network_ip.h"
#include "socketlayer.h"
#include "socketlayer_api.h"

#include "ControlMessages.h"

#ifndef ARPHRD_LOOPBACK
#define ARPHRD_LOOPBACK 772
#endif

//#define SL_DEBUG

/** COMMENTS
 1. all addresses and port number are in host order

 */
int SocketLayerFindNextFd(
    Node* node)
{
    int fd;
    SLData* slData = node->slData;

    for (fd = 0; fd < SL_MAX_SOCK_FDS; fd++)
    {
        if (!slData->socketList[fd])
            return fd;
    }

    return -1;
}

/***************************************************************************
 * ipv6.cpp functions, added to resolve inconsistency of struct in6_addr
 **************************************************************************/
//--------------------------------------------------------------------------
// FUNCTION             : SocketLayerIpv6GetPrefix
// PURPOSE             :: Gets the prefix from the specified address.
// PARAMETERS          ::
// + addr               : in6_addr* addr    : IPv6 Address pointer,
//                              the prefix of which to found.
// + prefix             : in6_addr* prefix  : IPv6 Address pointer
//                              the output prefix.
// + length             : int length        : IPv6 prefix length
// RETURN               : None
//--------------------------------------------------------------------------
void
SocketLayerIpv6GetPrefix(
    const in6_addr* addr,
    in6_addr* prefix,
    unsigned int length)
{
    int counter;

    // prefix length in octet
    int prefixLength = length / 8;
    unsigned int extraBit = length - prefixLength * 8;

    memset(prefix, 0, sizeof(in6_addr));

    for (counter = 0; counter < prefixLength; counter++)
    {
        prefix->s6_addr8[counter] = addr->s6_addr8[counter];
    }

    if (extraBit)
    {
        unsigned char temp = addr->s6_addr8[counter];

        temp >>= (8 - extraBit);
        temp <<= (8 - extraBit);

        prefix->s6_addr8[counter] = temp;
    }
}

// /**
// FUNCTION   :: SocketLayerIpv6GetPrefixLength
// LAYER      :: NETWORK
// PURPOSE    :: Get prefix length of an interface.
// PARAMETERS ::
//  +node           : Node* : Pointer to Node.
//  +interfaceIndex : int   : Inetrafce Index.
// RETURN     :: Prefix Length.
// **/
UInt32
SocketLayerIpv6GetPrefixLength(Node* node, int interfaceIndex)
{
    NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;

    char* pIfaceInfo = NULL;

    ERROR_Assert(
        interfaceIndex < node->numberInterfaces,
        "Invalid Interface Index");

    pIfaceInfo = (char*)ip->interfaceInfo[interfaceIndex]->ipv6InterfaceInfo;

    unsigned int* prefixLen =
        (unsigned int*)(pIfaceInfo + 3 * sizeof(in6_addr));
    return *prefixLen;
}

// XXX So if we have more than one open sockets with same port number?
// right now we are returning the same socket
int GetSLSocketFdFromPortNum(
        Node* node,
        unsigned short portNum,
        Address* remoteAddr,
        unsigned short remotePort)
{
    SLData* slData = node->slData;
    SLSocketData* sData;
    int fd, unconnected_fd = -1;

    for (fd=0; fd < SL_MAX_SOCK_FDS; fd++)
    {
        sData = slData->socketList[fd];
        if (sData)
        {
            if (sData->localAddress->sa_family == AF_INET)
            {
                sockaddr_in* tempLocal = (sockaddr_in*)sData->localAddress;
                sockaddr_in* tempRemote = (sockaddr_in*)sData->remoteAddress;
                if (tempLocal->sin_port == portNum)
                {
                    if (tempRemote->sin_addr.s_addr == 0
                        && tempRemote->sin_port == 0)
                    {
                        unconnected_fd = fd;
                    }
                    else if (remoteAddr
                             && tempRemote->sin_addr.s_addr ==
                                        htonl(remoteAddr->interfaceAddr.ipv4)
                             && tempRemote->sin_port == remotePort)
                    {
                        break;
                    }
                }
            }
            else
            {
                sockaddr_in6* tempLocal = (sockaddr_in6*)sData->localAddress;
                sockaddr_in6* tempRemote = (sockaddr_in6*)sData->remoteAddress;
                bool zero = true;
                for (int i = 0;
                     i < sizeof(tempRemote->sin6_addr.s6_addr);
                     i++)
                {
                    if (tempRemote->sin6_addr.s6_addr[i] != 0)
                    {
                        zero = false;
                    }
                }
                if (tempLocal->sin6_port == portNum)
                {
                    if (zero == true
                        && tempRemote->sin6_port == 0)
                    {
                        unconnected_fd = fd;
                    }
                    else if (remoteAddr
                             && memcmp(
                                 &tempRemote->sin6_addr,
                                 &remoteAddr->interfaceAddr.ipv6,
                                 sizeof(in6_addr)) == 0
                             && tempRemote->sin6_port == remotePort)
                    {
                        break;
                    }
                }
            }
        }
    }

    if (fd == SL_MAX_SOCK_FDS)
    {
        if (unconnected_fd > -1)
            return unconnected_fd;

        return -1;
    }
    return fd;
}

int GetSLSocketFdFromConnectionId(
        Node* node,
        int connId)
{
    SLData* slData = node->slData;
    SLSocketData* sData;
    int fd;

    for (fd=0; fd < SL_MAX_SOCK_FDS; fd++)
    {
        sData = slData->socketList[fd];
        if (sData &&
            (sData->connectionState == SL_SOCKET_CONNECTED ||
                sData->connectionState == SL_SOCKET_CLOSING) &&
            sData->connectionId == connId)
        {
            break;
        }
    }

    if (fd == SL_MAX_SOCK_FDS)
    {
        return -1;
    }
    return fd;
}

/*******************************************
 * Socket related API
 ******************************************/
int SL_socket(
    Node* node,
    int family,
    int protocol,
    int type)
{
    SLSocketData* sData;
    int fd;
    Address myDefaultAddress;
    NetworkType nettype = NETWORK_INVALID;

    sData = (SLSocketData*)MEM_malloc(sizeof(SLSocketData));

    if (!sData)
        return -1;

    memset(sData, 0, sizeof(SLSocketData));

    fd = SocketLayerFindNextFd(node);

    node->slData->socketList[fd] = sData;

    sData->family   = family;
    sData->protocol = protocol;
    sData->type     = type;

    sData->refCount = 1;
    sData->connectionState = SL_SOCKET_UNCONNECTED;
    sData->isNonBlocking = FALSE;

    sData->recvfromFunction = NULL;
    sData->recvFunction     = NULL;
    sData->listenFunction   = NULL;
    sData->connectFunction  = NULL;
    sData->sendFunction     = NULL;

    sData->acceptBacklogIndex = -1;

    if (family == AF_INET)
    {
        nettype = NETWORK_IPV4;
    }
    else if (family == AF_INET6)
    {
        nettype = NETWORK_IPV6;
    }
    else
    {
        return -1;
    }

    for (int i=0; i < node->numberInterfaces; i++)
    {
        NetworkGetInterfaceInfo(node, i, &myDefaultAddress, nettype);
        if (myDefaultAddress.networkType != NETWORK_INVALID)
        {
            break;
        }
    }
    if (myDefaultAddress.networkType == NETWORK_INVALID)
    {
        return -1;
    }
    if (family == AF_INET)
    {
        sData->localAddress = (sockaddr*)MEM_malloc(sizeof(sockaddr_in));
        memset(sData->localAddress, 0, sizeof(sockaddr_in));
        sData->remoteAddress = (sockaddr*)MEM_malloc(sizeof(sockaddr_in));
        memset(sData->remoteAddress, 0, sizeof(sockaddr_in));
        ((sockaddr_in*)(sData->localAddress))->sin_addr.s_addr =
                                htonl(myDefaultAddress.interfaceAddr.ipv4);
        ((sockaddr_in*)(sData->localAddress))->sin_port =
                                            APP_GetFreePort(node);
        APP_InserInPortTable(node,
                APP_SOCKET_LAYER,
                ((sockaddr_in*)(sData->localAddress))->sin_port);
    }
    else
    {
        sData->localAddress = (sockaddr*)MEM_malloc(sizeof(sockaddr_in6));
        memset(sData->localAddress, 0, sizeof(sockaddr_in6));
        sData->remoteAddress = (sockaddr*)MEM_malloc(sizeof(sockaddr_in6));
        memset(sData->remoteAddress, 0, sizeof(sockaddr_in6));
        ((sockaddr_in6*)(sData->localAddress))->sin6_addr =
                                        myDefaultAddress.interfaceAddr.ipv6;
        ((sockaddr_in6*)(sData->localAddress))->sin6_port =
                                            APP_GetFreePort(node);
        APP_InserInPortTable(node,
                APP_SOCKET_LAYER,
                ((sockaddr_in6*)(sData->localAddress))->sin6_port);
    }
    sData->localAddress->sa_family = family;

    sData->isUpaSocket = FALSE;
    sData->upaRecvBuffer = new queue<Message*>();
    sData->lastBufferRead = FALSE;

    return fd;
}

int SL_ioctl(
    Node* node,
    int fd,
    unsigned long cmd,
    void* arg,
    UInt32* argLength)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
    {
        return -1;
    }

    // Later on check based on CtrlMsg_0x3000 < cmd etc
    switch(cmd)
    {
        /* APPLICATION layer IOCTLs */
        /* TRANSPORT layer IOCTLs */
        /* NETWORK layer IOCTLs */
        case CtrlMsg_Network_GetRoute:
        {

            break;
        }
        case CtrlMsg_Network_SetRoute:
        {
            break;
        }

        /* MAC layer IOCTLs */
        case CtrlMsg_MAC_GetInterfaceList:
        {
            char* dataPtr = (char*)arg;
            int numInterfaces, thisInterfaceIndex;
            SLInterfaceInfo* ifInfo;
            NetworkDataIp* ip = (NetworkDataIp*)node->networkData.networkVar;

            numInterfaces = node->numberInterfaces;

            // The first interface is the loopback interface
            ifInfo = (SLInterfaceInfo*)dataPtr;
            dataPtr += sizeof(SLInterfaceInfo);

            sprintf(ifInfo->ifname, "lo");

            ifInfo->ifaddr.sin_family = AF_INET;
            ifInfo->ifaddr.address.ipv4.sin_addr.s_addr = 0x100007f;
            ifInfo->netmask.sin_family = AF_INET;
            ifInfo->netmask.address.ipv4.sin_addr.s_addr = htonl(0xFF000000);

            ifInfo->phyaddr.sa_family = ARPHRD_LOOPBACK;

#ifdef _WIN32
            ifInfo->flags = 0x1 | 0x40 | 0x8;
#else
            ifInfo->flags = (IFF_UP | IFF_RUNNING | IFF_LOOPBACK);
#endif
            ifInfo->ifindex = 0;
            ifInfo->metric = 1;
            ifInfo->mtu = 16436;

            // Now fill the other interfaces
            for (thisInterfaceIndex = 1;
                 thisInterfaceIndex <= numInterfaces;
                 thisInterfaceIndex++)
            {
                NetworkType type =
                    MAPPING_GetNetworkTypeFromInterface(node,
                                                    thisInterfaceIndex - 1);
                Address address =
                    MAPPING_GetInterfaceAddressForInterface(node,
                                                    node->nodeId,
                                                    type,
                                                    thisInterfaceIndex - 1);
                ifInfo = (SLInterfaceInfo*)dataPtr;
                dataPtr += sizeof(SLInterfaceInfo);

                sprintf(ifInfo->ifname, "qln%d", thisInterfaceIndex - 1);
                if (type == NETWORK_IPV4)
                {
                    ifInfo->ifaddr.sin_family = AF_INET;
                    ifInfo->netmask.sin_family = AF_INET;
                    AddressToSockaddr(
                        (sockaddr*)&(ifInfo->ifaddr.address.ipv4),
                        &address);
                    ifInfo->netmask.address.ipv4.sin_addr.s_addr = htonl(
                        NetworkIpGetInterfaceSubnetMask(node,
                                                   thisInterfaceIndex - 1));
                }
                else if (type == NETWORK_IPV6)
                {
                    ifInfo->ifaddr.sin_family = AF_INET6;
                    ifInfo->netmask.sin_family = AF_INET6;
                    AddressToSockaddr(
                        (sockaddr*)&(ifInfo->ifaddr.address.ipv6),
                        &address);
                    SocketLayerIpv6GetPrefix(
                                &(ifInfo->ifaddr.address.ipv6.sin6_addr),
                                &(ifInfo->netmask.address.ipv6.sin6_addr),
                                SocketLayerIpv6GetPrefixLength(node,
                                                thisInterfaceIndex - 1));
                }
                else
                {
                    continue;
                }

                ifInfo->phyaddr.sa_family = 1; //ARPHRD_ETHER;
                memcpy(&ifInfo->phyaddr.sa_data, "98989898", 8);


#ifdef _WIN32
                ifInfo->flags = 0x1 | 0x40 ;
#else
                ifInfo->flags = IFF_UP | IFF_RUNNING;
#endif
                ifInfo->ifindex = thisInterfaceIndex;
                ifInfo->metric = 1;
                ifInfo->mtu = 1500;
            }

            break;
        }
        case CtrlMsg_MAC_GetInterfaceAddress:
        {
            if (argLength == NULL)
            {
                return -1;
            }
            *argLength = GetSockaddrLength(sData->localAddress);
            if (*argLength == 0)
            {
                return -1;
            }
            if (arg != NULL)
            {
                memcpy(arg, sData->localAddress, *argLength);
            }
            break;
        }
        case CtrlMsg_MAC_SetInterfaceAddress:
        {
            if (argLength == NULL || *argLength == 0)
            {
                return -1;
            }
            if (sData->localAddress != NULL)
            {
                MEM_free(sData->localAddress);
            }
            sData->localAddress = (sockaddr*)MEM_malloc(*argLength);
            memcpy(sData->localAddress, arg, *argLength);
            break;
        }
        case CtrlMsg_MAC_GetInterfaceName:
        {
            break;
        }
        case CtrlMsg_MAC_SetInterfaceName:
        {
            break;
        }
        case CtrlMsg_MAC_SetBroadcastAddress:
        {
            break;
        }
        case CtrlMsg_MAC_GetNetmask:
        {
            break;
        }
        case CtrlMsg_MAC_SetNetmask:
        {
            break;
        }

        /* PHY layer IOCTLs */

        case CtrlMsg_PHY_SetInterfaceMTU:
        {
            break;
        }
        case CtrlMsg_PHY_GetInterfaceMTU:
        {
            break;
        }
        case CtrlMsg_PHY_SetTxPower:
        {
            //PHY_SetTxPower(node, phyIndex, *(double *)arg);
            break;
        }
        case CtrlMsg_PHY_GetTxPower:
        {
            //PHY_GetTxPower(node, phyIndex, (double *)arg);
            break;
        }
        case CtrlMsg_PHY_SetDataRate:
        {
            //PHY_SetDataRate(node, phyIndex, *(int *)arg);
            break;
        }
        case CtrlMsg_PHY_GetDataRate:
        {
            //int rate = PHY_GetTxDataRate(node, phyIndex);
            //*(int *)arg = rate;
            break;
        }
        case CtrlMsg_PHY_SetTxChannel:
        {
            break;
        }
        case CtrlMsg_PHY_GetTxChannel:
        {
            break;
        }
        case CtrlMsg_PHY_SetHWAddress:
        {
            break;
        }
        case CtrlMsg_PHY_GetHWAddress:
        {
            break;
        }
        case CtrlMsg_PHY_SetInterfaceFlags:
        {
            break;
        }
        case CtrlMsg_PHY_GetInterfaceFlags:
        {
            break;
        }
        case CtrlMsg_PHY_SetInterfaceMetric:
        {
            break;
        }
        case CtrlMsg_PHY_GetInterfaceMetric:
        {
            break;
        }

        /* UPA layer IOCTLs */
        case CtrlMsg_UPA_SetUpaSocket:
        {
            sData->isUpaSocket = TRUE;
            break;
        }
        case CtrlMsg_UPA_SetPhysicalAddress:
        {
            if (argLength == NULL || *argLength == 0)
            {
                return -1;
            }
            if (sData->upaPhysicalAddr != NULL)
            {
                MEM_free(sData->upaPhysicalAddr);
            }
            sData->upaPhysicalAddr = (sockaddr*)MEM_malloc(*argLength);
            memcpy(sData->upaPhysicalAddr, arg, *argLength);
            break;
        }
        case CtrlMsg_UPA_GetPhysicalAddress:
        {
            if (argLength == NULL)
            {
                return -1;
            }
            *argLength = GetSockaddrLength(sData->upaPhysicalAddr);
            if (*argLength == 0)
            {
                return -1;
            }
            if (arg != NULL)
            {
                memcpy(arg, sData->upaPhysicalAddr, *argLength);
            }
            break;
        }
        default:
            ERROR_ReportError("Undefined Control Message ioctl\n");
    }

    return 0;

}


int SL_sendto(
    Node* node,
    int fd,
    void* data,
    int dataSize,
    struct sockaddr* addr,
    int virtualLength)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    Address localAddr;
    Address destAddr;
    SockaddrToAddress(sData->localAddress, &localAddr);
    SockaddrToAddress(addr, &destAddr);
    short localPort = GetPortFromSockaddr(sData->localAddress);
    short dstPort = GetPortFromSockaddr(addr);
#ifdef SL_DEBUG
    char strLocalAddr[MAX_STRING_LENGTH];
    char strDstAddr[MAX_STRING_LENGTH];
    IO_ConvertIpAddressToString(&localAddr, strLocalAddr);
    IO_ConvertIpAddressToString(&destAddr, strDstAddr);
    printf("\t[SL] %d SL sending to <%s:%d> from <%s:%d>\n", node->nodeId,
        strDstAddr,
        dstPort,
        strDstAddr,
        localPort);
#endif

    Message* udpmsg = APP_UdpCreateMessage(
        node,
        localAddr,
        localPort,
        destAddr,
        dstPort,
        TRACE_SOCKETLAYER);

    APP_AddPayload(node, udpmsg, data, dataSize);

    APP_SetIsEmulationPacket(node, udpmsg);

    APP_UdpSend(node, udpmsg, PROCESS_IMMEDIATELY);

    return 0;
}


int SL_send(
    Node* node,
    int fd,
    void* data,
    int dataSize,
    int virtualLength)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    if (sData->connectionState != SL_SOCKET_CONNECTED)
        return -1;

    if (sData->type == SOCK_DGRAM)
    {
        return SL_sendto(node,
                         fd,
                         data,
                         dataSize,
                         sData->remoteAddress,
                         virtualLength);
    }
    else if (sData->type == SOCK_STREAM)
    {
        Message*  tcpmsg = APP_TcpCreateMessage(
            node, sData->connectionId, TRACE_SOCKETLAYER);

        APP_AddPayload(node, tcpmsg, data, dataSize);

        APP_SetIsEmulationPacket(node, tcpmsg);

        APP_TcpSend(node, tcpmsg);

    }
    else
    {
        assert(0);
    }
    return -1;
}

int SL_read(
    Node* node,
    int fd,
    void* buf,
    int n)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    return 0;
}




int SL_recvfrom(
    Node* node,
    int fd,
    void* buf,
    int n,
    int flags,
    struct sockaddr* addr,
    int* addr_len)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    if (!sData->isNonBlocking)
        ERROR_ReportError("SL_recvfrom can be called on non-blocking sockets only\n");

    return 0;
}


int SL_recv(
    Node* node,
    int fd,
    void** buf,
    int n,
    int flags)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    if (sData->lastBufferRead)
    {
        MESSAGE_Free(node, sData->upaRecvBuffer->front());
        sData->upaRecvBuffer->pop();
    }

    if (sData->upaRecvBuffer->size() == 0)
    {
        sData->lastBufferRead = FALSE;
        return -1;
    }

    *buf = MESSAGE_ReturnPacket(sData->upaRecvBuffer->front());
    sData->lastBufferRead = TRUE;

    return MESSAGE_ReturnPacketSize(sData->upaRecvBuffer->front());
}



int SL_write(
    Node* node,
    int fd,
    void* buf,
    int n,
    int virtualLength)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    return 0;
}


int SL_bind(
    Node* node,
    int fd,
    struct sockaddr* addr,
    int addr_len)
{
    SLSocketData* sData = node->slData->socketList[fd];

    short port = GetPortFromSockaddr(addr);
    short localPort = -1;

    if (!sData)
        return -1;

    if (sData->localAddress)
    {
        localPort = GetPortFromSockaddr(sData->localAddress);
    }

    if (port == 0)
    {
        return 0;
    }

    if (!APP_IsFreePort(node, port))
    {
#ifdef SL_DEBUG
        printf("PORT NOT FREE: %d at %d\n", addr_in->sin_port, node->nodeId);
#endif
        return -1;
    }

    if (localPort != -1)
    {
        APP_RemoveFromPortTable(node, localPort);
    }

    memcpy(sData->localAddress, addr, addr_len);

    APP_InserInPortTable(node, APP_SOCKET_LAYER, port);

    return 0;
}


int SL_connect(
    Node* node,
    int fd,
    struct sockaddr* addr,
    int addr_len)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    Address localAddr;
    Address destAddr;
    SockaddrToAddress(sData->localAddress, &localAddr);
    SockaddrToAddress(addr, &destAddr);

    short port = GetPortFromSockaddr(addr);
    short localPort = -1;

    if (sData->localAddress)
    {
        localPort = GetPortFromSockaddr(sData->localAddress);
    }

    if (sData->type == SOCK_DGRAM)
    {
        sData->connectionState = SL_SOCKET_CONNECTED;
        memcpy(sData->remoteAddress, addr, addr_len);
    }
    else if (sData->type == SOCK_STREAM)
    {
        assert(sData->connectFunction);

        sData->uniqueId = node->appData.uniqueId++;

        APP_TcpOpenConnection(
                node,
                (AppType)localPort,
                localAddr,
                localPort,
                destAddr,
                port,
                sData->uniqueId, // XXX
                PROCESS_IMMEDIATELY);
    }
    else
    {
        ERROR_ReportError("Unknown protocol type\n");
    }

    return 0;
}


int SL_accept(
    Node* node,
    int fd,
    struct sockaddr* addr,
    int* addr_len)
{
    SLSocketData* sData = node->slData->socketList[fd], *newSData;

    if (!sData)
        return -1;

    sockaddr* remoteAddr =
        sData->acceptBacklog[sData->acceptBacklogIndex].remoteAddr;
    *addr_len = GetSockaddrLength(remoteAddr);
    if (addr == NULL)
    {
        return 0;
    }
    int newFd;
    int localAddrLen = 0;

    assert(sData);

    newFd = SL_socket(node, sData->family, sData->protocol, sData->type);
    assert(newFd >= 0);

    newSData = node->slData->socketList[newFd];
    assert(newSData);

    memcpy(addr, remoteAddr, *addr_len);

    if (newSData->remoteAddress != NULL)
    {
        MEM_free(newSData->remoteAddress);
    }
    newSData->remoteAddress = (sockaddr*)MEM_malloc(*addr_len);
    memcpy(newSData->remoteAddress, addr, *addr_len);

    newSData->connectionState   = SL_SOCKET_CONNECTED;
    newSData->connectionId      =
        sData->acceptBacklog[sData->acceptBacklogIndex].connectionId;
    newSData->recvfromFunction  = sData->recvfromFunction;
    newSData->recvFunction      = sData->recvFunction;
    newSData->connectFunction   = sData->connectFunction;
    newSData->listenFunction    = sData->listenFunction;
    newSData->sendFunction      = sData->sendFunction;

    localAddrLen = GetSockaddrLength(sData->localAddress);

    if (newSData->localAddress != NULL)
    {
        MEM_free(newSData->localAddress);
    }
    newSData->localAddress = (sockaddr*)MEM_malloc(localAddrLen);
    memcpy(newSData->localAddress, sData->localAddress, localAddrLen);

    newSData->upaPhysicalAddr = NULL;
    sData->acceptBacklogIndex --;

    return newFd;
}


#ifndef _WIN32
int SL_poll(
    Node* node,
    struct pollfd* fds,
    nfds_t nfds,
    int timeout)
{

}
#endif


int SL_getsockname(
    Node* node,
    int fd,
    struct sockaddr* addr,
    int* addr_len)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    if (addr == NULL)
        return -1;

    *addr_len = GetSockaddrLength(sData->localAddress);
    memcpy(addr, sData->localAddress, *addr_len);

    return 0;
}


int SL_getpeername(
    Node* node,
    int fd,
    struct sockaddr* addr,
    int* addr_len)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    *addr_len = GetSockaddrLength(sData->remoteAddress);
    memcpy(addr, sData->remoteAddress, *addr_len);

    return 0;
}


int SL_listen(
    Node* node,
    int fd,
    int backlog)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    Address localAddr;
    SockaddrToAddress(sData->localAddress, &localAddr);

    short localPort = -1;

    if (sData->localAddress)
    {
        localPort = GetPortFromSockaddr(sData->localAddress);
    }

    assert(sData->listenFunction);

    if (sData->type == SOCK_STREAM)
    {
        if (sData->connectionState == SL_SOCKET_LISTENING)
            return 0;
        sData->connectionState = SL_SOCKET_LISTENING;
        APP_TcpServerListen(
                node,
                (AppType)localPort,
                localAddr,
                localPort);
    }
    return 0;
}

void
freeSData(SLSocketData* sData)
{
    if (sData->localAddress != NULL)
    {
        MEM_free(sData->localAddress);
    }
    if (sData->remoteAddress != NULL)
    {
        MEM_free(sData->remoteAddress);
    }
    if (sData->upaPhysicalAddr != NULL)
    {
        MEM_free(sData->upaPhysicalAddr);
    }
    MEM_free(sData);
}

int SL_shutdown(
    Node* node,
    int fd)
{
    SLSocketData* sData = node->slData->socketList[fd];
    APP_TcpCloseConnection(node, sData->connectionId);
    return 0;
}

int SL_close(
    Node* node,
    int fd)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return 0;

    UInt16* portPtr = NULL;

    if (sData->localAddress)
    {
        if (sData->localAddress->sa_family == AF_INET)
        {
            portPtr = &((sockaddr_in*)(sData->localAddress))->sin_port;
        }
        else
        {
            portPtr = &((sockaddr_in6*)(sData->localAddress))->sin6_port;
        }
    }

    sData->refCount--;

    if (sData->refCount)
    {
        return 0;
    }

    if (sData->type == SOCK_DGRAM)
    {
        APP_RemoveFromPortTable(node, *portPtr);
        node->slData->socketList[fd] = NULL;
        freeSData(sData);
        return 0;
    }

    // XXX default to TCP
    switch(sData->connectionState)
    {
        case SL_SOCKET_UNCONNECTED:
        case SL_SOCKET_LISTENING: /* TODO: Should detach the inpcb */
        {
                APP_RemoveFromPortTable(node, *portPtr);
                node->slData->socketList[fd] = NULL;
                freeSData(sData);
                break;
        }
        case SL_SOCKET_CONNECTED:
        {
            sData->connectionState = SL_SOCKET_CLOSING;
            APP_TcpCloseConnection(node, sData->connectionId);
            break;
        }
        case SL_SOCKET_CLOSING:
        {
            // If no other socket is using this port number, then release it
            unsigned short portNum = *portPtr;
            *portPtr = 0;

            if (GetSLSocketFdFromPortNum(node, portNum, 0, 0) < 0)
                APP_RemoveFromPortTable(node, portNum);

            node->slData->socketList[fd] = NULL;
            freeSData(sData);
            break;
        }
        default:
        {
            assert(0);
        }
    }
    return 0;
}


int SL_getsockopt(
    Node* node,
    int fd,
    int level,
    int optname,
    void* optval,
    int* optlen)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;
    return 0;

}


int SL_setsockopt(
    Node* node,
    int fd,
    int level,
    int optname,
    void* optval,
    int optlen)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;
    return 0;

}


int SL_fcntl(
    Node* node,
    int fd,
    int cmd,
    int arg)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    return 0;
}


int SL_recvmsg(
    Node* node,
    int fd,
    struct msghdr* msg,
    int flags)
{
    SLSocketData* sData = node->slData->socketList[fd];

    if (!sData)
        return -1;

    return 0;
}


int SL_select(
    Node* node,
    int nfds,
    fd_set* readfds,
    fd_set* writefds,
    fd_set* exceptfds,
    struct timeval* timeout)
{
    return 0;

    return 0;
}

void SL_fork(
    Node* node,
    int fd)
{
    SLSocketData* sData = node->slData->socketList[fd];

    assert(sData);

    sData->refCount++;
}

/*****************************************************
 * SocketLayer
 ****************************************************/

void SocketLayerInit(
    Node* node,
    const NodeInput* nodeInput)
{
    SLData* slData;

    slData = (SLData*)MEM_malloc(sizeof(SLData));
    node->slData = slData;

    for (int i=0; i < SL_MAX_SOCK_FDS; i++)
        slData->socketList[i] = NULL;

    slData->nextPortNum = SL_START_PORT_NUM;

}

BOOL SocketLayerProcessEvent(
    Node* node,
    Message* msg)
{
    int fd;
    SLSocketData* sData = NULL;

    switch(msg->eventType)
    {
        case MSG_APP_TimerExpired:
        {
            if (MESSAGE_GetProtocol(msg) != APP_SOCKET_LAYER)
                return FALSE;

            SLTimerData* info = (SLTimerData*)MESSAGE_ReturnInfo(msg);

            info->timerFunction(node, info->data);
            if (info->timerType == SL_TIMER_REPEAT)
                MESSAGE_Send(node, msg, info->interval);
            else
                MESSAGE_Free(node, msg);

            break;
        }
        case MSG_APP_FromTransport:
        {
            // XXX check AF_INET and udp
            UdpToAppRecv* info = (UdpToAppRecv*)MESSAGE_ReturnInfo(msg);
            unsigned short portNum = info->destPort;

            fd = GetSLSocketFdFromPortNum(node,
                        portNum,
                        &info->sourceAddr,
                        info->sourcePort);

            if (fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            if (sData->recvfromFunction)
            {
                sockaddr* addr = (sockaddr*)
                    MEM_malloc(AddressToSockaddr(NULL, &info->sourceAddr));
                AddressToSockaddr(addr, &info->sourceAddr);
                addr->sa_family = sData->family;
                SetPortFromSockaddr(addr, info->sourcePort);
                if (sData->connectionState == SL_SOCKET_UNCONNECTED)
                {
                    sData->recvfromFunction(node,
                            fd,
                            MESSAGE_ReturnPacket(msg),
                            MESSAGE_ReturnPacketSize(msg),
                            addr);
                }
                else if (sData->connectionState == SL_SOCKET_CONNECTED)
                {
                    sData->recvfromFunction(node,
                            fd,
                            MESSAGE_ReturnPacket(msg),
                            MESSAGE_ReturnPacketSize(msg),
                            NULL);
                }
                else
                {
                    assert(0);
                }
                MEM_free(addr);

            }
            else if (sData->recvFunction)
            {
                sData->recvFunction(node,
                            fd,
                            MESSAGE_ReturnPacket(msg),
                            MESSAGE_ReturnPacketSize(msg));
            }
            else
            {
#ifdef SL_DEBUG
                printf("[SL] No receive function defined\n");
#endif
            }

            break;
        }
        case MSG_APP_FromTransListenResult:
        {
            TransportToAppListenResult* result = (TransportToAppListenResult*)
                            MESSAGE_ReturnInfo(msg);

            unsigned short portNum = result->localPort;

            fd = GetSLSocketFdFromPortNum(node, portNum, 0, 0);
            if (fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            //assert(result->connectionId >= 0);
            if (result->connectionId < 0)
            {
#ifdef SL_DEBUG
                printf("[SL %d] ERROR Connection id is %d\n",
                       node->nodeId,
                       result->connectionId);
#endif
            }

            if (sData->listenFunction)
                sData->listenFunction(node, fd);

            break;
        }
        case MSG_APP_FromTransOpenResult:
        {
            TransportToAppOpenResult* result = (TransportToAppOpenResult*)
                            MESSAGE_ReturnInfo(msg);
            unsigned short portNum = result->localPort;
            UInt32 remoteAddrLen = 0;

            fd = GetSLSocketFdFromPortNum(node, portNum, 0, 0);
            if (fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            if (sData->connectionState == SL_SOCKET_LISTENING)
            {
                // this is the server socket
                if (sData->acceptFunction)
                {
                    if (sData->acceptBacklogIndex < SL_MAX_ACCEPT_BACKLOG - 1)
                    {
                        sockaddr* remoteAddr = (sockaddr*)
                            MEM_malloc(
                                AddressToSockaddr(NULL, &result->remoteAddr));
                        AddressToSockaddr(remoteAddr, &result->remoteAddr);

                        SetPortFromSockaddr(remoteAddr,
                                            result->remotePort,
                                            &remoteAddrLen);

                        sData->acceptBacklogIndex++;

                        sData->acceptBacklog[sData->acceptBacklogIndex].
                                                                remoteAddr =
                            (sockaddr*)MEM_malloc(remoteAddrLen);
                        memcpy(
                            sData->acceptBacklog[sData->acceptBacklogIndex].
                                                                remoteAddr,
                            remoteAddr,
                            remoteAddrLen);

                        sData->acceptBacklog[sData->acceptBacklogIndex].
                                                               connectionId =
                                result->connectionId;

                        sData->acceptFunction(node, fd);
                        MEM_free(remoteAddr);
                    }
                    else
                    {
#ifdef SL_DEBUG
                        printf("[SL] Not enough room to store incoming"
                               " listen at node %d\n",
                               node->nodeId);
#endif
                    }

                }
                else
                {
#ifdef SL_DEBUG
                    printf("[SL] listenFunction not defined\n");
#endif
                }

            }
            else if (sData->connectionState == SL_SOCKET_UNCONNECTED)
            {
                // this is the client socket
                if (sData->connectFunction)
                {
                    sockaddr* remoteAddr = (sockaddr*)
                        MEM_malloc(
                            AddressToSockaddr(NULL, &result->remoteAddr));
                    AddressToSockaddr(remoteAddr, &result->remoteAddr);

                    SetPortFromSockaddr(remoteAddr,
                                        result->remotePort,
                                        &remoteAddrLen);

                    if (result->connectionId >= 0)
                    {
                        sData->connectionState = SL_SOCKET_CONNECTED;
                        sData->connectionId = result->connectionId;

                        if (sData->remoteAddress != NULL)
                        {
                            MEM_free(sData->remoteAddress);
                        }
                        sData->remoteAddress =
                                            (sockaddr*)MEM_malloc(remoteAddrLen);

                        memcpy(sData->remoteAddress, remoteAddr, remoteAddrLen);
                        sData->connectFunction(node,
                                    fd,
                                    remoteAddr);
                    }
                    else
                    {
                        sData->connectFunction(node,
                                    fd,
                                    NULL);
                    }

                    MEM_free(remoteAddr);
                }
                else
                {
#ifdef SL_DEBUG
                    printf("[SL] connectFunction not defined\n");
#endif
                    assert(0);
                }
            }
            else
            {
#ifdef SL_DEBUG
                printf("[SL %d] Incorrect socket connectionstate = %d."
                       " fd=%d\n",
                       node->nodeId,
                       sData->connectionState,
                       fd);
#endif
            }

            break;
        }
        case MSG_APP_FromTransDataReceived:
        {
            TransportToAppDataReceived* result = (TransportToAppDataReceived*)
                            MESSAGE_ReturnInfo(msg);

            fd = GetSLSocketFdFromConnectionId(node, result->connectionId);
            if (fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            /* UPA Specific: if we got data from client, BEFORE the upa client
                can register the newly accepted socket. */
            if (sData->isUpaSocket)
            {
                if (sData->upaPhysicalAddr == NULL)
                {
                    sData->upaRecvBuffer->push(msg);
                    return TRUE;
                }
            }
            else
            {
                assert(0);
            }

            if (sData->recvfromFunction)
            {
#if 0
                struct sockaddr_in addr;
                addr.sin_family = sData->family;
                addr.sin_addr.s_addr = sData->remoteAddress.sin_addr.s_addr;
                addr.sin_port = sData->remoteAddress.sin_port;
#endif

                sData->recvfromFunction(node,
                            fd,
                            MESSAGE_ReturnPacket(msg),
                            MESSAGE_ReturnPacketSize(msg),
                            NULL);

            }
            else if (sData->recvFunction)
            {
                sData->recvFunction(node,
                            fd,
                            MESSAGE_ReturnPacket(msg),
                            MESSAGE_ReturnPacketSize(msg));

            }
            else
            {
#ifdef SL_DEBUG
                printf("[SL] No receive function defined\n");
#endif
            }

            break;
        }
        case MSG_APP_FromTransDataSent:
        {
            TransportToAppDataSent* result = (TransportToAppDataSent*)
                            MESSAGE_ReturnInfo(msg);

            fd = GetSLSocketFdFromConnectionId(node, result->connectionId);
            if (fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            if (sData->sendFunction)
            {
                sData->sendFunction(node, fd, result->length);
            }

            break;
        }
        case MSG_APP_FromTransCloseResult:
        {
            TransportToAppCloseResult* result = (TransportToAppCloseResult*)
                            MESSAGE_ReturnInfo(msg);

            fd = GetSLSocketFdFromConnectionId(node, result->connectionId);
            if (fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];

            if (sData->connectionState == SL_SOCKET_CLOSING)
            {
                SL_close(node, fd);
            }
            else
            {
                //sData->connectionState = SL_SOCKET_CLOSING;
                //APP_TcpCloseConnection(node, sData->connectionId);
                if (sData->isUpaSocket)
                {
                    if (sData->upaPhysicalAddr == NULL)
                    {
                        sData->upaRecvBuffer->push(msg);
                        return TRUE;
                    }
                    else
                    {
                        sData->recvfromFunction(node, fd, NULL, 0, NULL);
                    }

                    break;
                }

                if (sData->recvfromFunction)
                {
                    sData->recvfromFunction(node, fd, NULL, 0, NULL);
                }
                else if (sData->recvFunction)
                {
                    sData->recvFunction(node, fd, NULL, 0);
                }
                else
                {
                    assert(0);
                }
            }
            break;
        }
        default:
        {
            return FALSE;
        }
    }

    MESSAGE_Free(node, msg);

    return TRUE;
}

void SocketLayerSetTimer(
    Node* node,
    SLTimerType timerType,
    struct timeval* interval,
    void (*timerFunction)(Node*, void*),
    void* data)
{
    Message* msg;
    SLTimerData* info;

    msg = MESSAGE_Alloc(node,
                APP_LAYER,
                APP_SOCKET_LAYER,
                MSG_APP_TimerExpired);

    MESSAGE_InfoAlloc(node, msg, sizeof(data) + sizeof(timerFunction));
    info = (SLTimerData*)MESSAGE_ReturnInfo(msg);

    info->data = data;
    info->timerFunction = timerFunction;
    info->timerType = timerType;
    info->interval =
                interval->tv_sec * SECOND + interval->tv_usec * MICRO_SECOND;

    MESSAGE_Send(node, msg, info->interval);
}


void SocketLayerRegisterCallbacks(
    Node* node,
    int fd,
    void (*recvfromFunction)(Node*, int, char*, int, struct sockaddr*),
    void (*recvFunction)(Node*, int, char*, int),
    void (*listenFunction)(Node*, int),
    void (*acceptFunction)(Node*, int),
    void (*connectFunction)(Node*, int, struct sockaddr*),
    void (*sendFunction)(Node*, int, int))
{
    SLSocketData* sData = node->slData->socketList[fd];

    assert(sData);

    if (recvfromFunction)
        sData->recvfromFunction = recvfromFunction;

    if (recvFunction)
        sData->recvFunction     = recvFunction;

    if (listenFunction)
        sData->listenFunction   = listenFunction;

    if (acceptFunction)
        sData->acceptFunction   = acceptFunction;

    if (connectFunction)
        sData->connectFunction  = connectFunction;

    if (sendFunction)
        sData->sendFunction     = sendFunction;
}

void SocketLayerStoreLocalData(
    Node* node,
    int fd,
    void* data)
{
    SLSocketData* sData = node->slData->socketList[fd];
    assert(sData);

    sData->localData = data;
}

void* SocketLayerGetLocalData(
    Node* node,
    int fd)
{
    SLSocketData* sData = node->slData->socketList[fd];
    assert(sData);

    return sData->localData;
}
