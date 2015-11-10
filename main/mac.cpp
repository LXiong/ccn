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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// API
#include "main.h"
#include "api.h"
#include "partition.h"
#include "mac.h"
#include "qualnet_error.h"
#include "context.h"

// Developer library
#include "mac_802_3.h"
#include "mac_link.h"
#include "network_ip.h"
#include "network_dualip.h"
#include "ipv6.h"
#include "mac_satcom.h"
#include "mac_arp.h"
#include "mac_llc.h"
#include "mac_background_traffic.h"
#include "app_dhcp.h"

#ifdef ENTERPRISE_LIB
#include "routing_hsrp.h"
#include "mac_switch.h"
#include "mac_switched_ethernet.h"
#include "mpls.h"
#endif // ENTERPRISE_LIB

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#ifdef WIRELESS_LIB
#include "mac_aloha.h"
#include "mac_csma.h"
#include "mac_dot11.h"
#include "mac_generic.h"
#include "mac_maca.h"
#include "mac_tdma.h"
#include "mac_cellular.h"
#endif // WIRELESS_LIB

#ifdef ALE_ASAPS_LIB
#include "mac_ale.h"
#endif // ALE_ASAPS_LIB

#ifdef MILITARY_RADIOS_LIB
#include "phy_fcsc.h"
#include "mac_fcsc.h"
#include "mac_fcsc_csma.h"
#include "mac_link11.h"
#include "mac_link16.h"

#endif // MILITARY_RADIOS_LIB

#ifdef CELLULAR_LIB
#include "mac_gsm.h"
#endif

#ifdef ADDON_LINK16
#include "link16_mac.h"
#endif // ADDON_LINK16

#ifdef ADVANCED_WIRELESS_LIB
#include "mac_dot16.h"
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
#include "layer2_lte.h"
#include "epc_lte.h"
#endif // LTE_LIB

#ifdef SENSOR_NETWORKS_LIB
#include "mac_802_15_4.h"
#endif  // SENSOR_NETWORKS_LIB

#ifdef SATELLITE_LIB
#include "ane_mac.h"
#include "mac_satellite_bentpipe.h"
#endif // SATELLITE_LIB

#ifdef PARALLEL //Parallel
#include "parallel.h"
#include "phy_abstract.h"
#endif //endParallel

#ifdef CYBER_LIB
#include "mac_wormhole.h"
#include "app_jammer.h"
#include "mac_security.h"
#include "certificate_wtls.h"

#ifdef DO_ECC_CRYPTO
extern MPI caEccKey[12];
#endif // DO_ECC_CRYPTO
#define DEBUG_NETIA 0
#endif // CYBER_LIB

// debug option for Random Fault
#define RAND_FAULT_DEBUG 0

#undef DEBUG
#define DEBUG 0
#define DEBUG_HW_ADDRESS 0

#if defined(NETSNMP_INTERFACE)
#include "netSnmpAgent.h"
#endif

#ifdef ADDON_BOEINGFCS
// Taken from network_ip.h
#include "network_ces_inc.h"
#include "network_ces_inc_sincgars.h"
#include "network_ces_inc_eplrs.h"

#include "mac_ces_wnw_mdl.h"
#include "mac_wnw_main.h"
#include "mac_ces_wintncw.h"
#include "mac_ces_winthnw.h"
#include "mac_ces_wintgbs.h"
#include "mac_ces_eplrs.h"
#include "mac_ces_sincgars.h"
#include "link_ces_sincgars_sip.h"
#include "srw_qualnet_shim.h"
#include "srw-nexus.h"
#endif // ADDON_BOEINGFCS

#ifdef SOCKET_INTERFACE
#include "socket-interface.h"
#endif

#ifdef PARALLEL //Parallel
static void
MacUpdateRemoteNodeInterfaceCount(PartitionData* partitionData,
                                  NodeId         nodeID) {
    Node* node = MAPPING_GetNodePtrFromHash(partitionData->remoteNodeIdHash,
                                            nodeID);
    node->numberInterfaces++;
}
#endif //endParallel


// ------------------------------------------------------------------------
// Start :: MAC Adresses Configuration
// ------------------------------------------------------------------------

//---------------------------------------------------------------------------
// FUNCTION           : hexToDec
// PURPOSE            : Convert one byte Hex number to decimal number.
// RETURN             : return decimal number for valid hex digit and
//                      -1 for invalid hex digit
//---------------------------------------------------------------------------

static
int hexToDec(char chr)
{
    const char* str = "0123456789ABCDEF";
    char modifiedChar = (char) toupper(chr);

    int index = -1;
    int i=0;

    for (i = 0; i < 16; i++)
    {
        if (str[i] == modifiedChar)
        {
            index=i;
            break;
        }
    }
    return index;
}


//---------------------------------------------------------------------------
// FUNCTION           : decToHex
// PURPOSE            : Convert one byte decimal number to hex number.
// RETURN             : return correspondig hex digit string for
//                      one byte decimal number
//---------------------------------------------------------------------------

//static
char* decToHex(int dec)
{
    static char hexStr[2];
    const char *str="0123456789ABCDEF";

    hexStr[1] = str[(dec & 15)];
    hexStr[0] = str[(dec >> 4)];

    return hexStr;
}


//---------------------------------------------------------------------------
// FUNCTION           : MAC_PrintMacAddr
// PURPOSE            : Prints interface Mac Address
// PARAMETERS         :
// RETURN             : None
// ASSUMPTION         : Byte separator is always '-'
// --------------------------------------------------------------------------

void MAC_PrintMacAddr(Mac802Address *macAddr)
{
    printf("%s-", decToHex(macAddr->byte[0]));
    printf("%s-", decToHex(macAddr->byte[1]));
    printf("%s-", decToHex(macAddr->byte[2]));
    printf("%s-", decToHex(macAddr->byte[3]));
    printf("%s-", decToHex(macAddr->byte[4]));
    printf("%s", decToHex(macAddr->byte[5]));
}

// /**
// FUNCTION   :: MAC_PrintHWAddr
// LAYER      :: MAC
// PURPOSE    :: Prints interface hardware Address
// PARAMETERS ::
// + macAddr : MacHWAddress* : Pointer to hardware structure
// RETURN     :: void : NULL
// **/
void MAC_PrintHWAddr(
    MacHWAddress* macAddr)
{
    int i=0;

    for (i = 0; i < macAddr->hwLength -1; i++)
    {
        printf("%s-", decToHex(macAddr->byte[i]));
    }
    if (i != 0 )
    {
        printf("%s ", decToHex(macAddr->byte[i]));
    }
}

// /**
// FUNCTION   :: GetNodeIdFromMacAddress
// LAYER      :: MAC
// PURPOSE    :: Return first four bytes of mac address
// PARAMETERS ::
// + byte[] : unsigned char : Mac address string
// RETURN     :: NodeId : unsigned int
// **/
NodeId GetNodeIdFromMacAddress (unsigned char byte[])
{
    return (((unsigned int)(byte[3]) << 24) |
           ((unsigned int)(byte[2]) << 16) |
           ((unsigned int)(byte[1]) << 8) |
           (unsigned int)(byte[0]));

}

// /**
// FUNCTION   :: SetNodeIdInMacAddress
// LAYER      :: MAC
// PURPOSE    :: Sets first four bytes of mac address
// PARAMETERS ::
// + byte[] : unsigned char : Mac address string
// + nodeId : NodeId : unsigned int, node id of node
// RETURN     :: void : NULL
// **/
inline void SetNodeIdInMacAddress (unsigned char byte[], NodeId nodeId)
{
    byte[0] = ((unsigned char )(nodeId));
    byte[1] = ((unsigned char )(nodeId >> 8));
    byte[2] = ((unsigned char )(nodeId >> 16));
    byte[3] = ((unsigned char )(nodeId >> 24));
}

// /**
// FUNCTION   :: SetInterfaceIndexInMacAddress
// LAYER      :: MAC
// PURPOSE    :: Sets Interface index in mac address
// PARAMETERS ::
// + byte[] : unsigned char : Mac address string
// + interfaceIndex : unsigned int : interfaceIndex of a node
// RETURN     :: void : NULL
// **/
inline void SetInterfaceIndexInMacAddress (unsigned char byte[],
                                           unsigned int interfaceIndex)
{
    byte[5] = ((unsigned char )(interfaceIndex));
}

// /**
// FUNCTION   :: GetInterfaceIndexFromMacAddress
// LAYER      :: MAC
// PURPOSE    :: Get Interface index from mac address
// PARAMETERS ::
// + byte[] : unsigned char : Mac address string
// RETURN     ::unsigned int : interfaceindex
// **/
inline unsigned int GetInterfaceIndexFromMacAddress (unsigned char byte[])
{
    return ((unsigned int) byte[5]);
}

// /**
// FUNCTION   :: MacSetDefaultHWAddress
// LAYER      :: MAC
// PURPOSE    :: Set Default interface Hardware Address of node
// PARAMETERS ::
// + nodeId : NodeId : Id of the input node
// + macAddr : MacHWAddress* : Pointer to hardware structure
// + interfaceIndex : int : Interface on which the hardware address set
// RETURN     :: void : NULL
// **/

void MacSetDefaultHWAddress(
    NodeId nodeId,
    MacHWAddress* macAddr,
    int interfaceIndex)
{

    macAddr->hwLength = MAC_ADDRESS_DEFAULT_LENGTH;
    macAddr->hwType = HW_TYPE_ETHER;

    (*macAddr).byte = (unsigned char*) MEM_malloc(
                          sizeof(unsigned char)*MAC_ADDRESS_DEFAULT_LENGTH);

    SetNodeIdInMacAddress(macAddr->byte, nodeId);

    macAddr->byte[4] = 0x00;
    SetInterfaceIndexInMacAddress(macAddr->byte, interfaceIndex);
}

// /**
// FUNCTION   :: MAC_SetFourByteMacAddress
// LAYER      :: MAC
// PURPOSE    :: Set Default interface Hardware Address of node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : MacHWAddress* : Pointer to hardware structure
// + interfaceIndex : int : Interface on which the hardware address set
// RETURN     :: void : NULL
// **/
static
void MAC_SetFourByteMacAddress(
                    Node* node,
                    NodeId nodeId,
                    MacHWAddress* macAddr,
                    int interfaceIndex)
{
    NodeAddress nodeAddress = 0;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
    macAddr->hwLength = MAC_IPV4_LINKADDRESS_LENGTH;
    macAddr->hwType = IPV4_LINKADDRESS;
    (*macAddr).byte = (unsigned char*) MEM_malloc(
                          sizeof(unsigned char)*IPV4_LINKADDRESS);
    if (ip->interfaceInfo[interfaceIndex]->interfaceType == NETWORK_IPV6)
    {
        nodeAddress =
            MAPPING_CreateIpv6LinkLayerAddr(nodeId, interfaceIndex);
    }
    else
    {
      nodeAddress = NetworkIpGetInterfaceAddress(node, interfaceIndex);
    }

    SetNodeIdInMacAddress(macAddr->byte, nodeAddress);
}

// /**
// FUNCTION   :: MAC_SetTwoByteMacAddress
// LAYER      :: MAC
// PURPOSE    :: Set Default interface Hardware Address of node
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : MacHWAddress* : Pointer to hardware structure
// + interfaceIndex : int : Interface on which the hardware address set
// RETURN     :: void : NULL
// **/
void MAC_SetTwoByteMacAddress(
            Node* node,
            MacHWAddress* macAddr,
            int interfaceIndex)
{
    UInt16 tmpNodeId;
    macAddr->hwLength = MAC_NODEID_LINKADDRESS_LENGTH;
    macAddr->hwType = HW_NODE_ID;
    (*macAddr).byte = (unsigned char*) MEM_malloc(
                            sizeof(unsigned char) *
                                    MAC_NODEID_LINKADDRESS_LENGTH);
    tmpNodeId = (UInt16)node->nodeId;
    memcpy(macAddr->byte, &tmpNodeId, MAC_NODEID_LINKADDRESS_LENGTH);
}

void MAC_SetFourteenBitMacAddress(Node *node,
                                  MacHWAddress* macAddr,
                                  int interfaceIndex)
{
    MAC_SetTwoByteMacAddress(node, macAddr, interfaceIndex);
    UInt16 addr14 = (UInt16)node->nodeId && 0x3fff;
    memcpy(macAddr->byte, &addr14, MAC_NODEID_LINKADDRESS_LENGTH);
}

// /**
// FUNCTION   :: MacValidateAndSetHWAddress
// LAYER      :: MAC
// PURPOSE    :: Validate MAC Address String after fetching from user
// PARAMETERS ::
// + macAddrStr : char* : Pointer to address string
// + macAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: void : NULL
// **/
void MacValidateAndSetHWAddress(
    char* macAddrStr,
    MacHWAddress* macAddr)
{
    int i = 0;
    unsigned short hwlength = 0;
    BOOL flag = FALSE; // flag will set to TRUE for valid mac address
    int strlength = (int)strlen(macAddrStr);

    MacHWAddress tempHWAddr;
    tempHWAddr.hwLength = MAX_MACADDRESS_LENGTH;
    tempHWAddr.byte = (unsigned char*) MEM_malloc(tempHWAddr.hwLength);


    for (i = 0; i < strlength; i += 3)
    {
        if (isxdigit(macAddrStr[0]) && isxdigit(macAddrStr[1]))
        {
            int rightNibble = hexToDec(macAddrStr[1]);
            int leftNibble = hexToDec(macAddrStr[0]);

            int byteValue = rightNibble + (16 * leftNibble);

            tempHWAddr.byte[hwlength] = (unsigned char)byteValue;

            if ((i +3) < strlength)
            {
              ERROR_Assert((macAddrStr[2] == ':') || (macAddrStr[2] == '-'),
                    "Invalid MAC Address");
            }

            macAddrStr = macAddrStr + 3;
            hwlength++;

            if ((flag == FALSE) && (byteValue != 255) && (byteValue != 0))
            {
                flag = TRUE;
            }
        }
        else
        {
            ERROR_Assert(FALSE, "Invalid Mac Address ");
        }
    }

    (*macAddr).byte = (unsigned char*) MEM_malloc(hwlength);
     macAddr->hwLength = hwlength;
     memcpy(macAddr->byte, tempHWAddr.byte, macAddr->hwLength);


    ERROR_Assert(flag, "MAC Address "
        "should be neither broadcast address, nor null address");
}


// /**
// FUNCTION   :: DefaultMacHWAddressToIpv4Address
// LAYER      :: MAC
// PURPOSE    :: Retrieve the IP Address  from Default HW Address .
//               Default HW address is equal to 6 bytes
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: NodeAddress : Ip address
// **/

NodeAddress DefaultMacHWAddressToIpv4Address(
    Node* node,
    MacHWAddress* macAddr)
{
    if (MAC_IsBroadcastHWAddress(macAddr))
    {
        return ANY_DEST;
    }
    else
    {
        unsigned int nodeId = 0;
        int interfaceIndex = GetInterfaceIndexFromMacAddress(macAddr->byte);

        nodeId = GetNodeIdFromMacAddress(macAddr->byte);

        return MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                               node, nodeId, interfaceIndex);
    }
}

// /**
// FUNCTION   :: DefaultMac802AddressToIpv4Address
// LAYER      :: MAC
// PURPOSE    :: Retrieve  IP address from.Mac802Address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : Mac802Address* : Pointer to hardware address structure
// RETURN     :: NodeAddress : Ipv4 Address
// **/

NodeAddress DefaultMac802AddressToIpv4Address(
    Node* node,
    Mac802Address* macAddr)
{
    if (MAC_IsBroadcastMac802Address(macAddr))
    {
        return ANY_DEST;
    }
    else
    {
        unsigned int nodeId = GetNodeIdFromMacAddress(macAddr->byte);
        int interfaceIndex = GetInterfaceIndexFromMacAddress(macAddr->byte);

        return MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                               node, nodeId, interfaceIndex);
    }
}


// /**
// FUNCTION   :: MAC_VariableHWAddressToFourByteMacAddress
// LAYER      :: MAC
// PURPOSE    :: Retrieve  IP address from.MacHWAddress of type
//               IPV4_LINKADDRESS
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: NodeAddress : Ipv4 Address
// **/

NodeAddress MAC_VariableHWAddressToFourByteMacAddress (
    Node* node,
    MacHWAddress* macAddr)
{
    if (macAddr->hwLength != MAC_IPV4_LINKADDRESS_LENGTH ||
        macAddr->hwType == HW_TYPE_UNKNOWN)
    {
        return 0;
    }

    if (MAC_IsBroadcastHWAddress(macAddr))
    {
        return ANY_DEST;
    }
    else
    {
        return GetNodeIdFromMacAddress(macAddr->byte);
    }
}

// /**
//--------------------------------------------------------------------------
// FUNCTION   MacHWAddressToIpv4Address
// PURPOSE:   This functions converts variable length Mac address
//            to IPv4 address It checks the type of hardware address
//            and based on that conversion is done.
// PARAMETERS   Node *node
//                  Pointer to node which indicates the host
//              MacHWAddress* macAddr
//                  Pointer to MacHWAddress Structure.
//                    IP address is found for Mac addres
// RETURN       Node Address (IP address)
//--------------------------------------------------------------------------
// **/


NodeAddress MacHWAddressToIpv4Address(
                Node *node,
                int interfaceIndex,
                MacHWAddress* macAddr)
{

    if (MAC_IsBroadcastHWAddress(macAddr))
    {
            return ANY_DEST;
    }
    else if (ArpIsEnable(node,interfaceIndex))
    {
        return ReverseArpTableLookUp(node, interfaceIndex, macAddr);
    }
    else
    {
            switch(macAddr->hwType)
            {
                case HW_TYPE_ETHER:
                {
                      Mac802Address mac802Addr;
                      ConvertVariableHWAddressTo802Address(node,
                                                           macAddr,
                                                           &mac802Addr);
                      return DefaultMac802AddressToIpv4Address(node,
                                                               &mac802Addr);
                      break;
                }
                case IPV4_LINKADDRESS:
                {
                    return MAC_VariableHWAddressToFourByteMacAddress (
                                                               node,macAddr);

                    break;
                }
                case HW_NODE_ID:
                {
                    UInt16 nId;
                    memcpy(&nId, macAddr->byte, MAC_NODEID_LINKADDRESS_LENGTH);
                    return MAPPING_GetDefaultInterfaceAddressFromNodeId(
                            node, nId);
                    break;
                }
                default:
                            ERROR_Assert(FALSE, "Unsupported hardware type");
                    break;
            }
    }

    return 0;
}
// /**
// FUNCTION   :: IPv4AddressToDefaultMac802Address
// LAYER      :: MAC
// PURPOSE    :: Retrieve the Mac802Address  from IP address.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + index : int : Interface Index
// + ipv4Address : NodeAddress : Ipv4 address from which the
//                                hardware address resolved.
// + macAddr : Mac802Address* : Pointer to Mac802address structure
// RETURN     :: void : NULL
// **/

void IPv4AddressToDefaultMac802Address(
    Node *node,
    int index,
    NodeAddress ipv4Address,
    Mac802Address *macAddr)
{

    Address interfaceAddr;
    interfaceAddr.interfaceAddr.ipv4 = ipv4Address;
    interfaceAddr.networkType = NETWORK_IPV4;

     unsigned int nodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                node,
                                interfaceAddr);

     unsigned int interfaceIndex =
                          MAPPING_GetInterfaceIndexFromInterfaceAddress(
                                                node,
                                                interfaceAddr);

    if (interfaceIndex == INVALID_MAPPING ||
        nodeId == INVALID_MAPPING ||
        ipv4Address == ANY_DEST ||
        ipv4Address == 0 ||
        (ipv4Address == NetworkIpGetInterfaceBroadcastAddress(node, index)))
    {
        memset(macAddr->byte, 255, MAC_ADDRESS_LENGTH_IN_BYTE);
    }
    else
    {
        SetNodeIdInMacAddress(macAddr->byte, nodeId);
        macAddr->byte[4] = 0x00;
        SetInterfaceIndexInMacAddress(macAddr->byte, interfaceIndex);
    }
}


// /**
// FUNCTION   :: IPv4AddressToDefaultHWAddress
// LAYER      :: MAC
// PURPOSE    :: Retrieve the MacHWAddress of 6 bytes  from IP address.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macAddr : MacHWAddress* : Pointer to hardware address structure
// + ipv4Address : NodeAddress : Ipv4 address from which the
//                                hardware address resolved.
// RETURN     :: void : NULL
// **/

void IPv4AddressToDefaultHWAddress(
    Node *node,
    int index,
    NodeAddress ipv4Address,
    MacHWAddress *macAddr)
{
     macAddr->hwLength = MAC_ADDRESS_LENGTH_IN_BYTE;
     macAddr->hwType = HW_TYPE_ETHER;

    Address interfaceAddr;
    interfaceAddr.interfaceAddr.ipv4 = ipv4Address;
    interfaceAddr.networkType = NETWORK_IPV4;

    unsigned int nodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                node,
                                interfaceAddr);
    unsigned int interfaceIndex =
                          MAPPING_GetInterfaceIndexFromInterfaceAddress(
                                node,
                                interfaceAddr);

    if (interfaceIndex == INVALID_MAPPING ||
        nodeId == INVALID_MAPPING ||
        ipv4Address == ANY_DEST  ||
        ipv4Address == 0 ||
        (ipv4Address == NetworkIpGetInterfaceBroadcastAddress(node, index)))
    {
        memset(macAddr->byte, 255, MAC_ADDRESS_LENGTH_IN_BYTE);
    }
    else
    {
        SetNodeIdInMacAddress(macAddr->byte, nodeId);
        macAddr->byte[4] = 0;
        SetInterfaceIndexInMacAddress(macAddr->byte, interfaceIndex);
    }

}

// /**
// FUNCTION   :: IPv4AddressToHWAddress
// LAYER      :: MAC
// PURPOSE    ::Converts IP address.To MacHWAddress
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// +interfaceIndex : int interfcae index of a node
// + macAddr : MacHWAddress* : Pointer to hardware address structure
// + ipv4Address : NodeAddress : Ipv4 address from which the
//                                hardware address resolved.
// RETURN     :: BOOL : Returns False when conversion fails
// **/
BOOL IPv4AddressToHWAddress(
    Node *node,
    int interfaceIndex,
    Message* msg,
    NodeAddress ipv4Address,
    MacHWAddress* macAddr)
{

    BOOL isResolved = FALSE;

    switch (node->macData[interfaceIndex]->macProtocol)
    {
        case MAC_PROTOCOL_CSMA:
        case MAC_PROTOCOL_MACA:
        case MAC_PROTOCOL_802_3:
        case MAC_PROTOCOL_ALOHA:
        case MAC_PROTOCOL_GENERICMAC:
        case MAC_PROTOCOL_SWITCHED_ETHERNET:
        case MAC_PROTOCOL_TDMA:
        case MAC_SWITCH:
        case MAC_PROTOCOL_DOT11:
        case MAC_PROTOCOL_DOT16:
#ifdef LTE_LIB
        case MAC_PROTOCOL_LTE:
#endif // LTE_LIB
        case MAC_PROTOCOL_LINK:
        {
                (*macAddr).byte = (unsigned char*) MEM_malloc(
                          sizeof(unsigned char)*MAC_ADDRESS_DEFAULT_LENGTH);
                IPv4AddressToDefaultHWAddress(node, interfaceIndex,
                                              ipv4Address, macAddr);
                isResolved = TRUE;
            break;
        }

        case MAC_PROTOCOL_802_11:   //change 802.11,Dot11 and Dot 16 to
        //case MAC_PROTOCOL_DOT11:   //upper after changes in those protocols
        //case MAC_PROTOCOL_DOT16:
        case MAC_PROTOCOL_TADIL_LINK16:
        case MAC_PROTOCOL_TADIL_LINK11:
        case MAC_PROTOCOL_SPAWAR_LINK16:
        case MAC_PROTOCOL_GSM:
        case MAC_PROTOCOL_ALE:
        case MAC_PROTOCOL_SATCOM:
        case MAC_PROTOCOL_SATELLITE_BENTPIPE:
        case MAC_PROTOCOL_ANE:
        case MAC_PROTOCOL_MODE5:
        case MAC_PROTOCOL_CELLULAR:
        case MAC_PROTOCOL_WORMHOLE:
        case MAC_PROTOCOL_CES_WNW_MDL:
        case MAC_PROTOCOL_BOEING_GENERICMAC:
        case MAC_PROTOCOL_CES_EPLRS:
        case MAC_PROTOCOL_CES_SINCGARS:
        case MAC_PROTOCOL_CES_SRW:
        case MAC_PROTOCOL_CES_SRW_PORT:
        case MAC_PROTOCOL_CES_WINTNCW:
        case MAC_PROTOCOL_CES_WINTHNW:
        case MAC_PROTOCOL_CES_WINTGBS:
         {
                MAC_FourByteMacAddressToVariableHWAddress(node,
                                                          interfaceIndex,
                                                          macAddr,
                                                          ipv4Address);
                isResolved = TRUE;
                break;
         }
#ifdef SENSOR_NETWORKS_LIB
        case MAC_PROTOCOL_802_15_4:
        {
            MAC802_15_4IPv4AddressToHWAddress(node, ipv4Address, macAddr);
            isResolved = TRUE;
            break;
        }
#endif // SENSOR_NETWORKS_LIB
        default:
            isResolved = FALSE;
            ERROR_Assert(FALSE, "Unknown Mac Protocol,"
                              "Can not convert IP Address to Mac Address");
    }

    return isResolved;

}



// /**
// FUNCTION   :: ConvertVariableHWAddressTo802Address
// LAYER      :: MAC
// PURPOSE    :: Convert Variable Hardware address to Mac 802 addtess
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// + mac802Addr : Mac802Address* : Pointer to mac 802 address structure
// RETURN     :: Bool :
// **/
BOOL ConvertVariableHWAddressTo802Address(
            Node *node,
            MacHWAddress* macHWAddr,
            Mac802Address* mac802Addr)
{
    if (macHWAddr->hwLength == 6)
    {
        memcpy(mac802Addr->byte, macHWAddr->byte, macHWAddr->hwLength);
        return TRUE;
    }
    return FALSE;
}

// /**
// FUNCTION   :: Convert802AddressToVariableHWAddress
// LAYER      :: MAC
// PURPOSE    :: Convert Mac 802 addtess to Variable Hardware address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// + mac802Addr : Mac802Address* : Pointer to mac 802 address structure
// RETURN     :: Bool :
// **/
void Convert802AddressToVariableHWAddress (
            Node *node,
            MacHWAddress* macHWAddr,
            Mac802Address* mac802Addr)
{
        macHWAddr->hwLength = MAC_ADDRESS_DEFAULT_LENGTH;
        macHWAddr->hwType = HW_TYPE_ETHER;
        (*macHWAddr).byte = (unsigned char*) MEM_malloc(
                          sizeof(unsigned char)*MAC_ADDRESS_DEFAULT_LENGTH);
        memcpy(macHWAddr->byte, mac802Addr->byte, MAC_ADDRESS_DEFAULT_LENGTH);
}

//--------------------------------------------------------------------------
// FUNCTION   MAC_FourByteMacAddressToVariableHWAddress
// PURPOSE:     Convert 4 byte address to the variable hardware address
// PARAMETERS  Node *node
//               Pointer to node which indicates the host
//            MacHWAddress* macAddr
//               Pointer to source MacHWAddress Structure
//             NodeAddress nodeAddr
//              Ip address
// RETURN      Null
//--------------------------------------------------------------------------


void MAC_FourByteMacAddressToVariableHWAddress(
                    Node* node,
                    int interfaceIndex,
                    MacHWAddress* macAddr,
                    NodeAddress nodeAddr)
{
    macAddr->hwLength = MAC_IPV4_LINKADDRESS_LENGTH;
    macAddr->hwType = IPV4_LINKADDRESS;
    (*macAddr).byte = (unsigned char*) MEM_malloc(
                          sizeof(unsigned char)*IPV4_LINKADDRESS);

     SetNodeIdInMacAddress(macAddr->byte, nodeAddr);
}


static void setHWType(char *hwTypeStr,
                      unsigned short* hwType)
{

     if (strcmp(hwTypeStr, "ETHERNET") == 0)
                {
                    *hwType = HW_TYPE_ETHER;
                }
     else if (strcmp(hwTypeStr, "ATM") == 0)
                {
                    *hwType = HW_TYPE_ATM;
                }
                else
                {
                    ERROR_Assert(FALSE,
                        "Invalid Hardware type or does not support the"
                        " Current Version");
                }

}

static BOOL readInterfaceMacAddress(
           Node* node,
           NodeId nodeId,
           int interfaceIndex,
           const NodeInput* nodeInput,
           MacHWAddress* macAddr,
           BOOL isArpEnabled,
           Address ipAddr)
{


    char tempString[MAX_STRING_LENGTH] = {0};
    BOOL retVal = FALSE;
    int length = 0;

    IO_ReadString(node,
                  nodeId,
                  interfaceIndex,
                  nodeInput,
                  "MAC-ADDRESS-TYPE",
                  &retVal,
                  tempString);

    if (!retVal)
    {
        return FALSE;
    }


    setHWType(tempString, &macAddr->hwType);
    macAddr->hwLength = MAC_ADDRESS_DEFAULT_LENGTH;

    IO_ReadInt(node,
               nodeId,
               interfaceIndex,
               nodeInput,
               "MAC-ADDRESS-LENGTH",
               &retVal,
               &length);

    if (!retVal)
    {
        return FALSE;
    }
    else
    {
        switch(macAddr->hwType)
        {

          case HW_TYPE_ETHER:
          case HW_TYPE_ATM:
          {
             if (length != MAC_ADDRESS_DEFAULT_LENGTH)
             {
                ERROR_ReportWarning(
                  "Ethernet MAC addresses should have 6 byte MAC Address");
                macAddr->hwLength = (unsigned short) length;
                break;
             }
          }
          default:
               macAddr->hwLength = (unsigned short) length;
               break;
        }
    }

   ERROR_Assert(length > 0, "Hardware address length should"
                            "be greater than 0");

    IO_ReadString(node,
                  nodeId,
                  interfaceIndex,
                   nodeInput,
                  "MAC-ADDRESS",
                  &retVal,
                  tempString);

     if (!retVal)
     {
        return FALSE;
     }
     else if (!isArpEnabled && ipAddr.networkType == NETWORK_IPV4)
     {
        ERROR_Assert(FALSE,
            "ARP is not Enabled, Can not use User Configured Mac Address");
        return FALSE;
     }

     MacValidateAndSetHWAddress(tempString, macAddr);

     return TRUE;
}

// /**
// FUNCTION   :: MacConfigureHWAddress
// LAYER      :: MAC
// PURPOSE    :: Configure interface MAC address
//               User can specify MAC addresse using following syntax
//               MAC-ADDRESS-CONFIG-FILE     ./default.mac-address
//               In default.mac-address file:
//               Syntax:
//               <nodeId> <Interface-index> <Mac-Address> [length] [hwType]
//               where Mac-Address must be in hex format.
//               e.g.
//                     1    0   23:4f:5C:aa:FE:B2
//                     2    0   5C-AA-FE-23-4F-C2
//
//               If MAC-ADDRESS-CONFIG-FILE is not specified then
//               following convention will be used to generate
//               interface MAC addresse:
//               For 6 byte First 32 bits is set to node id, next 8 bits
//               is 0, next 8 bits is interface id.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex : int : Interface of the node
// + nodeInput : const NodeInput* : Ponter to the user specific file
// + macAddr : MacHWAddress* : Pointer to the hardware structure
// RETURN     :: void : NULL
// **/
static
void MacConfigureHWAddress(
    Node* node,
    NodeId nodeId,
    int interfaceIndex,
    const NodeInput* nodeInput,
    MacHWAddress* macAddr,
    const char *macProtocolName,
    NetworkType networkType,
    Address ipAddr)
{
    BOOL retVal = FALSE;
    BOOL isArpValue = FALSE;
    BOOL readMacAddress = TRUE;
    BOOL macAddrConfigured = FALSE;
    int i = 0;
    NodeInput macAddrInput;
    unsigned short  hwLength = 0;
    unsigned short  hwType = 0;

    if (networkType == NETWORK_IPV4)
    {
        char value[MAX_STRING_LENGTH]={0};

        IO_ReadString(node,
                      nodeId,
                      interfaceIndex,
                      nodeInput,
                      "ARP-ENABLED",
                      &isArpValue,
                      value);

        if (isArpValue && strcmp(value, "YES") == 0)
         {
            readMacAddress = TRUE;
         }
         else
         {
            readMacAddress = FALSE;
         }
    }

        macAddrConfigured = readInterfaceMacAddress(node,
                                                    nodeId,
                                                    interfaceIndex,
                                                    nodeInput,
                                                    macAddr,
                                                    readMacAddress,
                                                    ipAddr);

        if (!macAddrConfigured)
        {

            IO_ReadCachedFile(node,
                              nodeId,
                              interfaceIndex,
                              nodeInput,
                              "MAC-ADDRESS-CONFIG-FILE",
                              &retVal,
                              &macAddrInput);

            if (retVal &
                (!readMacAddress && ipAddr.networkType == NETWORK_IPV4))
            {
                  ERROR_Assert(FALSE,
               "ARP is not Enabled,Can not use User Configured Mac Address");
            }

            if (retVal)
            {
                for (i = 0; i < macAddrInput.numLines; i++)
                {
                    int item = 0;
                    NodeId tempNodeId = 0;
                    int intfIndex = -1;
                    char nodeIdStr[MAX_STRING_LENGTH] = {0};
                    char intfIndexStr[MAX_STRING_LENGTH] = {0};
                    char hwLengthStr[MAX_STRING_LENGTH] = {0};
                    char macAddrStr[MAX_STRING_LENGTH] = {0};
                    char hwTypeStr[MAX_STRING_LENGTH] = {0};

                    item = sscanf(macAddrInput.inputStrings[i],
                                  "%s %s %s %s %s",
                                  nodeIdStr, intfIndexStr,
                                  hwLengthStr, hwTypeStr,
                                  macAddrStr);


                    if (item == MAC_CONFIGURATION_ATTRIBUTE -1)
                    {
                         hwLength = (unsigned short) strtoul(hwLengthStr,
                                                           NULL, 10);
                        if (hwLength == 0)
                        {
                            hwLength = MAC_ADDRESS_DEFAULT_LENGTH;
                            setHWType(hwLengthStr, &hwType);
                            memcpy(macAddrStr, hwTypeStr, strlen(hwTypeStr));
                        }
                        else
                        {
                            hwType = (unsigned short)HW_TYPE_ETHER;
                            memcpy(macAddrStr, hwTypeStr, strlen(hwTypeStr));
                        }
                    }
                    else if (item == MAC_CONFIGURATION_ATTRIBUTE -2)
                    {
                        hwLength = MAC_ADDRESS_DEFAULT_LENGTH;
                        hwType = HW_TYPE_ETHER;
                        memcpy(macAddrStr, hwLengthStr, strlen(hwLengthStr));
                    }
                    else if (item <= MAC_CONFIGURATION_ATTRIBUTE -3)
                    {
                        ERROR_Assert(FALSE,
                            "Invalid number of entry in mac address file");
                    }
                    else if (item == MAC_CONFIGURATION_ATTRIBUTE)
                    {
                        setHWType(hwTypeStr, &hwType);
                        hwLength = (unsigned short) strtoul(hwLengthStr,
                                                          NULL, 10);
                    }

                    tempNodeId = (NodeId) strtoul(nodeIdStr, NULL, 10);
                    intfIndex = (int) strtoul(intfIndexStr, NULL, 10);

                    if ((tempNodeId == nodeId) &&
                        (intfIndex == interfaceIndex))
                    {
                        macAddr->hwLength = hwLength;
                        macAddr->hwType = hwType;
                        // Validate macAddrStr and feed macAddr
                        MacValidateAndSetHWAddress(macAddrStr, macAddr);
                        if (macAddr->hwLength == MAC_IPV4_LINKADDRESS_LENGTH)
                        {
                            macAddr->hwType = IPV4_LINKADDRESS;
                        }
                        macAddrConfigured = TRUE;
                        break;
                    }
            }// end for
        }   // end if
    }   // ending reading of MAC-ADDRESS-CONFIG-FILE

    if (!macAddrConfigured)
    {
        if (strcmp(macProtocolName, "MAC802.3")==0 ||
           strcmp(macProtocolName, "MACDOT11")==0 ||
           strcmp(macProtocolName, "ALOHA") == 0  ||
           strcmp(macProtocolName, "TDMA") == 0 ||
           strcmp(macProtocolName, "MACA") == 0 ||
           strcmp(macProtocolName, "GENERICMAC") == 0 ||
           strcmp(macProtocolName, "CSMA") == 0 ||
           strcmp(macProtocolName, "MACDOT11e") == 0 ||
           strcmp(macProtocolName, "MAC802.16") == 0 ||
           strcmp(macProtocolName, "MAC-LTE") == 0 ||
           strcmp(macProtocolName, "Link") == 0 ||
//#ifdef ENTERPRISE_LIB
           strcmp(macProtocolName, "SWITCHED-ETHERNET") == 0)
//#endif
        {

                MacSetDefaultHWAddress(
                    nodeId,
                    macAddr,
                    interfaceIndex);
        }

        else if (strcmp(macProtocolName, "MAC-LINK-11") == 0 ||
                strcmp(macProtocolName, "MAC-LINK-16") == 0 ||
                strcmp(macProtocolName, "ALE") == 0 ||
                strcmp(macProtocolName, "GSM") == 0 ||
                strncmp(macProtocolName, "FCSC-", 5) == 0 ||
                strcmp(macProtocolName, "MAC-SPAWAR-LINK16") == 0 ||
                strcmp(macProtocolName, "SATTSM") == 0 ||
                strcmp(macProtocolName, "SATCOM") == 0 ||
                strcmp(macProtocolName, "ANE") == 0 ||
                strcmp(macProtocolName, "MODE5") == 0 ||
                strcmp(macProtocolName, "SATELLITE-BENTPIPE") == 0 ||
                strcmp(macProtocolName, "MAC802.11") == 0 ||
                strcmp(macProtocolName, "CELLULAR-MAC") == 0 ||
                strcmp(macProtocolName, "MAC-WORMHOLE") == 0 ||
                strcmp(macProtocolName, "USAP") == 0 ||
                strcmp(macProtocolName, "MAC-CES-WNW-MDL") == 0 ||
                strcmp(macProtocolName, "BOEING-GENERICMAC") == 0 ||
                strcmp(macProtocolName, "MAC-CES-WIN-T-GBS") == 0 ||
                strcmp(macProtocolName, "MAC-CES-WIN-T-NCW") == 0 ||
                strcmp(macProtocolName, "MAC-CES-WIN-T-HNW") == 0 ||
                strcmp(macProtocolName, "MAC-CES-SINCGARS") == 0 ||
                strcmp(macProtocolName, "MAC-CES-EPLRS") == 0 ||
                strcmp(macProtocolName, "MAC-CES-SIP") == 0)
        {
                MAC_SetFourByteMacAddress(
                    node,
                    nodeId,
                    macAddr,
                    interfaceIndex);
        }
        else if (strcmp(macProtocolName, "MAC802.15.4") == 0)
        {
            MAC_SetTwoByteMacAddress(
                        node,
                        macAddr,
                        interfaceIndex);
        }
        else if (strcmp(macProtocolName, "MAC-CES-SRW") == 0)
        {
            MAC_SetFourteenBitMacAddress(node,
                                         macAddr,
                                         interfaceIndex);
        }
        else
        {
                ERROR_Assert(FALSE,
                        "Invalid Mac Protocol or does not "
                        "have any supported Mac address type");
        }
    }

    if (DEBUG_HW_ADDRESS)
    {
        printf("Node %d Interface %d Mac Address : ",nodeId, interfaceIndex);
        MAC_PrintHWAddr(macAddr);
        printf("\n");
    }
}


// /**
// FUNCTION   :: MacGetHardwareType
// LAYER      :: MAC
// PURPOSE    :: Retrieve the Hardware Type.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex : int : interface whose mac type requires
// + type : unsigned short* : Pointer to hardware type
// RETURN     :: void : NULL
// **/

void MacGetHardwareType(
    Node* node,
    int interfaceIndex,
    unsigned short* type)
{
    MacHWAddress *hwAddr;
    hwAddr = &node->macData[interfaceIndex]->macHWAddr;
    *type = hwAddr->hwType;
}

// /**
// FUNCTION   :: MacGetHardwareLength
// LAYER      :: MAC
// PURPOSE    :: Retrieve the Hardware Length.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex : int : interface whose hardware length required
// + hwLength  : unsigned short  : Pointer to hardware string
// RETURN     :: void : NULL
// **/
void MacGetHardwareLength(
    Node* node,
    int interfaceIndex,
    unsigned short *hwLength)
{
    MacHWAddress *hwAddr;
    hwAddr = &node->macData[interfaceIndex]->macHWAddr;
    *hwLength = hwAddr->hwLength;
}

// /**
// FUNCTION   :: MacGetHardwareAddressString
// LAYER      :: MAC
// PURPOSE    :: Retrieve the Hardware Address String.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex : int : interface whose  hardware address retrieved
// +macAddressString  : unsigned char* : Pointer to hardware string
// RETURN     :: void : NULL
// **/
void MacGetHardwareAddressString(
    Node* node,
    int interfaceIndex,
    unsigned char* macAddressString)
{
    MacHWAddress *hwAddr;
    hwAddr = &node->macData[interfaceIndex]->macHWAddr;
    memcpy( macAddressString, hwAddr->byte, hwAddr->hwLength);
}
// ------------------------------------------------------------------------
// End :: MAC Adresses Configuration
// ------------------------------------------------------------------------

static void
MacSetInterface(Node* node, int interfaceIndex,
                Address* address,const NodeInput *nodeInput)
{
    BOOL wasFound = FALSE;
    BOOL readValue = FALSE;
    char yesOrNo[MAX_STRING_LENGTH];
    node->macData[interfaceIndex] =
        (MacData *)MEM_malloc(sizeof(MacData));

    memset(node->macData[interfaceIndex], 0, sizeof(MacData));

    node->macData[interfaceIndex]->interfaceIndex = interfaceIndex;
    for (int k(0); k < MAX_PHYS_PER_MAC; k++)
    {
      node->macData[interfaceIndex]->phyNumberArray[k] = -1;
    }

#ifdef ADDON_DB
    // if this interface is already enabled, then it should not
    // trigger a DB insert
    StatsDBInitializeMacStructure(node, interfaceIndex);
#endif

    IO_ReadBool(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "MAC-LAYER-STATISTICS",
        &wasFound,
        &readValue);

    if (wasFound && readValue)
    {
        node->macData[interfaceIndex]->stats = new STAT_MacStatistics(node);
    }
    else
    {
#ifdef ADDON_DB
        if (node->partitionData->statsDb)
        {
            if (node->partitionData->statsDb->statsAggregateTable->createMacAggregateTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB MAC_Aggregate table requires\n"
                    " MAC-LAYER-STATISTICS to be set to YES\n");
            }
            if (node->partitionData->statsDb->statsSummaryTable->createMacSummaryTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB MAC_Summary table requires\n"
                    " MAC-LAYER-STATISTICS to be set to YES\n");
            }
        }
#endif
    }
    MAC_EnableInterface(node, interfaceIndex);

    ListInit(node,
             &node->macData[interfaceIndex]->interfaceStatusHandlerList);

    assert(node->macData[interfaceIndex]->interfaceStatusHandlerList !=
                                                                    NULL);

    // Dynamic API
    std::string path;
    D_Hierarchy *h = &node->partitionData->dynamicHierarchy;

    if (h->CreateNodeInterfacePath(
            node,
            interfaceIndex,
            "fault",
            path))
    {
        h->AddObject(
            path,
            new D_Fault(node, interfaceIndex));
    }

    node->macData[interfaceIndex]->virtualMacAddress = 0;
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "LLC-ENABLED",
        &wasFound,
        yesOrNo);

    node->macData[interfaceIndex]->isLLCEnabled = FALSE;
    if (wasFound)
    {
        if (strcmp(yesOrNo, "NO") == 0)
        {
            node->macData[interfaceIndex]->isLLCEnabled = FALSE;
        }
        else
        if (strcmp(yesOrNo, "YES") == 0)
        {
            node->macData[interfaceIndex]->isLLCEnabled = TRUE;
        }
        else
        {
            node->macData[interfaceIndex]->isLLCEnabled = FALSE;
        }
    }
}


void MacAddNewInterface(
    Node *node,
    NodeAddress interfaceAddress,
    int numHostBits,
    int *interfaceIndex,
    const NodeInput *nodeInput,
    const char *macProtocolName,
    BOOL isNewInterface)
{
    Address addr;
    addr.networkType = NETWORK_IPV4;
    addr.interfaceAddr.ipv4 = interfaceAddress;

    NetworkIpAddNewInterface(
        node,
        interfaceAddress,
        numHostBits,
        interfaceIndex,
        nodeInput,
        isNewInterface);

    if (isNewInterface)
    {
        MacSetInterface(node, *interfaceIndex,&addr,nodeInput);

        // Configure Interface MAC Address
        MacConfigureHWAddress(node, node->nodeId, *interfaceIndex, nodeInput,
                                &node->macData[*interfaceIndex]->macHWAddr,
                                macProtocolName, NETWORK_IPV4, addr);
    }

    NetworkDataIp *ip = node->networkData.networkVar;
    IpInterfaceInfoType* intf =
        (IpInterfaceInfoType*)ip->interfaceInfo[*interfaceIndex];

#ifdef TRANSPORT_AND_HAIPE
    // Initialize HAIPE spec
    strcpy(intf->haipeSpec.name, "Undefined");
#endif // TRANSPORT_AND_HAIPE
#ifdef CYBER_LIB
    char buffer[MAX_STRING_LENGTH*4];
    char address[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;
    NodeAddress uniqueAddr = interfaceAddress;

    // Should count PHY turnaround time for the (wormhole victim) node?
    IO_ReadString(
        node,
        node->nodeId,
        *interfaceIndex,
        nodeInput,
        "WORMHOLE-VICTIM-COUNT-TURNAROUND-TIME",
        &retVal,
        buffer);

    if (!retVal || !strcmp(buffer, "NO"))
    {
        intf->countWormholeVictimTurnaroundTime = FALSE;
    }
    else if (!strcmp(buffer, "YES"))
    {
        intf->countWormholeVictimTurnaroundTime = TRUE;
        IO_ReadTime(
            node,
            node->nodeId,
            *interfaceIndex,
            nodeInput,
            "WORMHOLE-VICTIM-TURNAROUND-TIME",
            &retVal,
            &intf->wormholeVictimTurnaroundTime);

        if (!retVal)
        {
            intf->wormholeVictimTurnaroundTime = 0;
        }
    }

    // Determines the eavesdropping status
    IO_ReadString(
        node,
        node->nodeId,
        *interfaceIndex,
        nodeInput,
        "EAVESDROP-ENABLED",
        &retVal,
        buffer);

    if (!retVal || !strcmp(buffer, "NO"))
    {
        intf->eavesdropFile = NULL;
    }
    else if (!strcmp(buffer, "YES"))
    {
        char eavesdropFileName[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(intf->ipAddress, address);
        // Record file name is appended with corresponding IP address
        sprintf(eavesdropFileName, "default.eavesdrop.%s", address);

        intf->eavesdropFile = fopen(eavesdropFileName, "w");
        ERROR_Assert(intf->eavesdropFile != NULL,
                     "Error in creating eavesdrop file.\n");
        fprintf(intf->eavesdropFile,
                "time: ip_v ip_hl ip_tos ip_len ip_id "
                "flags(ip_reserved ip_dont_fragment ip_more_fragments) "
                "ip_fragment_offset ip_ttl ip_p ip_sum ip_src ip_dst\n\n");
    }
    else
    {
        ERROR_ReportError("EAVESDROP-ENABLED expects YES or NO");
    }

    // Determines the auditing status
    IO_ReadString(
        node,
        node->nodeId,
        *interfaceIndex,
        nodeInput,
        "AUDIT-ENABLED",
        &retVal,
        buffer);

    if (!retVal || !strcmp(buffer, "NO"))
    {
        intf->auditFile = NULL;
    }
    else if (!strcmp(buffer, "YES"))
    {
        char auditFileName[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(intf->ipAddress, address);
        sprintf(auditFileName, "default.audit.%s", address);

        intf->auditFile = fopen(auditFileName, "w");
        ERROR_Assert(intf->auditFile != NULL,
                     "Error in creating audit file.\n");
        fprintf(intf->auditFile, "time: ip_v ip_hl ip_tos ip_len ip_id "
                "flags(ip_reserved ip_dont_fragment ip_more_fragments) "
                "ip_fragment_offset ip_ttl ip_p ip_sum ip_src ip_dst\n\n");
    }
    else
    {
        ERROR_ReportError("AUDIT-ENABLED expects YES or NO");
    }

#ifdef DO_ECC_CRYPTO
    //------------------------------------------------------------------------
    // Now handle this interface's public key and private key:
    // (1) Create one if there is none.
    // (2) Test its validity if the file is already there.
    //------------------------------------------------------------------------
    char keyFileName[MAX_STRING_LENGTH], pubKeyFileName[MAX_STRING_LENGTH];
    FILE *keyFile, *pubKeyFile;
    int i;

    IO_ConvertIpAddressToString(
        node->networkData.networkVar->interfaceInfo[*interfaceIndex]->ipAddress,
        address);
    sprintf(keyFileName, "default.privatekey.%s", address);
    sprintf(pubKeyFileName, "default.publickey.%s", address);

    keyFile = fopen(keyFileName, "r");
    pubKeyFile = fopen(pubKeyFileName, "r");
    if (keyFile == NULL || pubKeyFile == NULL)
    {
        // The node's key file doesn't exist, create it
        keyFile = fopen(keyFileName, "w");
        sprintf(buffer, "Error in creating key file %s\n", keyFileName);
        ERROR_Assert(keyFile != NULL, buffer);

        pubKeyFile = fopen(pubKeyFileName, "w");
        sprintf(buffer, "Error in creating public key file %s\n",
                pubKeyFileName);
        ERROR_Assert(pubKeyFile != NULL, buffer);

        int invalid = 1;

        while (invalid)
        {
            fprintf(stderr,
                    "Generate Elliptic Curve Cryptosystem (ECC) key "
                    "for node %u\n", node->nodeId);

            // Choose the random nonce k
            intf->eccKey[11] = EccChooseRandomNumber(NULL,
                                                     QUALNET_ECC_KEYLENGTH);
            invalid = ecc_generate(PUBKEY_ALGO_ECC,
                                   QUALNET_ECC_KEYLENGTH,
                                   intf->eccKey,
                                   NULL);
        }

        /*
          well-known.E.p = sk.E.p = eccKey[0];
          well-known.E.a = sk.E.a = eccKey[1];
          well-known.E.b = sk.E.b = eccKey[2];
          well-known.E.G.x = sk.E.G.x = eccKey[3];
          well-known.E.G.x = sk.E.G.y = eccKey[4];
          well-known.E.G.x = sk.E.G.z = eccKey[5];
          well-known.E.G.x = sk.E.n = eccKey[6];
          pk.Q.x = sk.Q.x = eccKey[7];
          pk.Q.y = sk.Q.y = eccKey[8];
          pk.Q.z = sk.Q.z = eccKey[9];
          sk.d = eccKey[10];

          Amongst the parameters,
          d is the random secret, Q is supposed to be public key,
          so different Q's for different d's.

          The other parameters
          E.p, E.a, E.b, E.n, E.G.x, E.G.y, E.G.z are fixed and well-known
          for fixed-bit ECCs.

          For example, for 192-bit ECC:

          mpi_fromstr(E.p_,
          "0xfffffffffffffffffffffffffffffffeffffffffffffffff"))

          mpi_fromstr(E.a_,
          "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFC")) i.e., "-0x3"

          mpi_fromstr(E.b_,
          "0x64210519e59c80e70fa7e9ab72243049feb8deecc146b9b1"))

          mpi_fromstr(E.n_,
          "0xffffffffffffffffffffffff99def836146bc9b1b4d22831"))

          mpi_fromstr(G->x_,
          "0x188da80eb03090f67cbf20eb43a18800f4ff0afd82ff1012"))

          mpi_fromstr(G->y_,
          "0x07192b95ffc8da78631011ed6b24cdd573f977a11e794811"))

          G->z_ = 1;
        */

        // public key does NOT include sk.d
        for (i=0; i<10; i++)
        {
            fprintf(pubKeyFile, "0x");
            mpi_print(pubKeyFile, intf->eccKey[i], 1);
            fputc('\n', pubKeyFile);
        }
        // key has all 11 elements
        for (i=0; i<11; i++)
        {
            fprintf(keyFile, "0x");
            mpi_print(keyFile, intf->eccKey[i], 1);
            fputc('\n', keyFile);
        }
    }
    else
    {
        // The pre-generated key is already in the file,
        // use it rather than generate it.  This saves simulation time.
        for (i=0; i<11; i++)
        {
            if ((fgets(buffer, QUALNET_MAX_STRING_LENGTH, keyFile) == NULL)
                || (buffer[strlen(buffer)-1] != '\n'))
            {
                sprintf(buffer,
                        "Key file %s is corrupted. Please delete it.\n",
                        keyFileName);
                ERROR_ReportError(buffer);
            }
            buffer[strlen(buffer)-1] = '\0';

            intf->eccKey[i] = mpi_alloc_set_ui(0);
            mpi_fromstr(intf->eccKey[i], buffer);
        }
        // Choose the random nonce k
        intf->eccKey[11] =
            EccChooseRandomNumber(intf->eccKey[6]/*sk.n*/,
                                  QUALNET_ECC_KEYLENGTH);
        int invalid = test_ecc_key(PUBKEY_ALGO_ECC,
                                   intf->eccKey,
                                   QUALNET_ECC_KEYLENGTH);
        fprintf(stderr, "Node %u's ECC key is %s.\n",
                node->nodeId, invalid?"INVALID":"VALID");
        while (invalid)
        {
            fprintf(stderr,
                    "Generate Elliptic Curve Cryptosystem (ECC) key "
                    "for node %u\n", node->nodeId);
            // Choose the random nonce k
            mpi_free(intf->eccKey[11]);
            intf->eccKey[11] = EccChooseRandomNumber(NULL,
                                                     QUALNET_ECC_KEYLENGTH);
            invalid = ecc_generate(PUBKEY_ALGO_ECC,
                                   QUALNET_ECC_KEYLENGTH,
                                   intf->eccKey,
                                   NULL);
        }
    }
    fclose(keyFile);
    fclose(pubKeyFile);
#endif // DO_ECC_CRYPTO

    // Determines the eavesdropping status
    IO_ReadString(
        node,
        node->nodeId,
        *interfaceIndex,
        nodeInput,
        "CERTIFICATE-ENABLED",
        &retVal,
        buffer);

    if (!retVal || !strcmp(buffer, "NO"))
    {
        intf->certificate = NULL;
    }
    else if (!strcmp(buffer, "YES"))
    {
        IO_ReadString(
            node,
            node->nodeId,
            *interfaceIndex,
            nodeInput,
            "CERTIFICATE-FILE-LOG",
            &retVal,
            buffer);

        if (!retVal || !strcmp(buffer, "YES"))
        {
            intf->certificateFileLog = TRUE;
        }
        else if (!strcmp(buffer, "NO"))
        {
            intf->certificateFileLog = FALSE;
        }
        else
        {
            ERROR_ReportError("CERTIFICATE-FILE-LOG expects YES or NO");
        }

        //-------------------------------------------------------------
        // Now handle this node's certificate:
        // (1) Create one if there is none.
        // (2) Test its validity if the file is already there.
        //-------------------------------------------------------------
        char certFileName[MAX_STRING_LENGTH];
        FILE *certFile;

        byte cert[MAX_STRING_LENGTH];
        length_t certLength =0, signatureLength = 0;

        IO_ConvertIpAddressToString(
            node->networkData.networkVar->
            interfaceInfo[*interfaceIndex]->ipAddress,
            address);
        // File name is appended with corresponding IP address
        sprintf(certFileName, "default.certificate.%s", address);

        certFile = fopen(certFileName, "r");
        if (certFile == NULL)
        {
            // The node's certificate file doesn't exist, create it
#ifdef DO_ECC_CRYPTO
          RENEW_CERTIFICATE:
#endif // DO_ECC_CRYPTO
            if (intf->certificateFileLog)
            {
                certFile = fopen(certFileName, "w");
                sprintf(buffer,
                        "Error in creating certificate file %s\n",
                        certFileName);
                ERROR_Assert(certFile != NULL, buffer);
            }

            char now[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(node->getNodeTime(), now);
            char *dot = strchr(now, '.');
            *dot = 0;
            uint32 tend = (uint32)atoi(now);
            // valid for a year (assuming a simulation ends in a year)
            tend += 365*24*3600;
            certLength = (length_t)AssembleCompactCertificate(
                cert,
                address,
                (length_t)strlen(address),
#ifdef DO_ECC_CRYPTO
                &intf->eccKey[7],
#endif // DO_ECC_CRYPTO
                QUALNET_ECC_KEYLENGTH,
                0,
                tend);
            if (certLength > MAX_STRING_LENGTH)
            {
                sprintf(buffer, "Node %u's certificate is too long.\n",
                        node->nodeId);
                ERROR_ReportError(buffer);
            }
            // Convert bitstream to hexadecimal form
            bitstream_to_hexstr((byte *)buffer, cert, certLength);
            if (intf->certificateFileLog)
            {
                fprintf(certFile, "%s\n", buffer);
            }

#ifdef DO_ECC_CRYPTO
            // append the crypto signature
            WTLS_PrivateKey caSK;
            byte signature[MAX_STRING_LENGTH];

            caSK.key.ecKey.nbits = QUALNET_ECC_KEYLENGTH;
            mpi_free(caEccKey[11]);
            caEccKey[11] = EccChooseRandomNumber(NULL, QUALNET_ECC_KEYLENGTH);
            memcpy(caSK.key.ecKey.sk, caEccKey, 12*sizeof(MPI));
            SignCertificate(cert, certLength,
                            signature, &signatureLength,
                            SA_ECDSA_SHA, &caSK);

            bitstream_to_hexstr((byte *)buffer, signature, signatureLength);
            if (intf->certificateFileLog)
            {
                fprintf(certFile, "%s\n", buffer);
            }
#endif // DO_ECC_CRYPTO
            if (intf->certificateFileLog)
            {
                fclose(certFile);
            }
        }
        else
        {
            // The pre-generated certificate is already in the file,
            // use it rather than generate it.  This saves simulation time.
            if ((fgets(buffer, QUALNET_MAX_STRING_LENGTH, certFile) == NULL)
               || (buffer[strlen(buffer)-1] != '\n'))
            {
                sprintf(buffer,
                        "Certificate file %s is corrupted. "
                        "Please delete it.\n",
                        certFileName);
                ERROR_ReportError(buffer);
            }
            if (buffer[strlen(buffer)-2] == '\r')
            {
                buffer[strlen(buffer)-2] = '\0';
            }
            else
            {
            buffer[strlen(buffer)-1] = '\0';
            }
            certLength =
                (length_t)hexstr_to_bitstream(cert, 0, (byte *)buffer);

            char id[MAX_STRING_LENGTH];
            length_t idLength, keyBitLength;
            uint32 tstart, tend;
            DeassembleCompactCertificate(cert,
                                         id,
                                         &idLength,
#ifdef DO_ECC_CRYPTO
                                         &intf->eccKey[7],
#endif // DO_ECC_CRYPTO
                                         &keyBitLength,
                                         &tstart,
                                         &tend);
            if (strncmp(id, address, idLength) != 0)
            {
                sprintf(buffer,
                        "Certificate file %s is for ID `%s'. "
                        "Assume corrupted. Please delete it.\n",
                        certFileName, id);
                ERROR_ReportError(buffer);
            }

            char now[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(node->getNodeTime(), now);
            char *dot = strchr(now, '.');
            *dot = 0;
            uint32 nowSecond = (uint32)atoi(now);
            if (nowSecond < tstart || nowSecond > tend)
            {
                sprintf(buffer,
                        "Certificate file %s expired. "
                        "Please delete it.\n",
                        certFileName);
                ERROR_ReportError(buffer);
            }
#ifdef DO_ECC_CRYPTO
            byte signature[MAX_STRING_LENGTH];

            if ((fgets(buffer, QUALNET_MAX_STRING_LENGTH, certFile) == NULL)
               || (buffer[strlen(buffer)-1] != '\n'))
            {
                sprintf(buffer,
                        "Certificate file %s is corrupted. "
                        "Please delete it.\n",
                        certFileName);
                ERROR_ReportError(buffer);
            }
            buffer[strlen(buffer)-1] = '\0';
            signatureLength =
                hexstr_to_bitstream(signature, 0, (byte *)buffer);

            WTLS_PublicKey caPK;

            caPK.key.ecPublicKey.nbits = QUALNET_ECC_KEYLENGTH;
            memcpy(caPK.key.ecPublicKey.pk, caEccKey, 10*sizeof(MPI));
            caPK.key.ecPublicKey.pk[10] =
                EccChooseRandomNumber(NULL, QUALNET_ECC_KEYLENGTH);
            int invalid = VerifyCertificate(cert, certLength,
                                            signature, signatureLength,
                                            SA_ECDSA_SHA, &caPK);
            fprintf(stderr, "Node %u's certificate is %s.\n",
                    node->nodeId, invalid? "INVALID":"VALID");
            if (invalid)
            {
                goto RENEW_CERTIFICATE;
            }
            // Valid and print it
            if (DEBUG)
            {
                PrintCompactCertificate(stderr, cert);
            }
#endif // DO_ECC_CRYPTO
            fclose(certFile);
        }
        fflush(stderr);

        // Record the certificate
        intf->certificate = (char *)MEM_malloc(certLength+signatureLength);
        memcpy(intf->certificate, cert, certLength+signatureLength);
        intf->certificateLength = certLength+signatureLength;
    }
    else
    {
        ERROR_ReportError("CERTIFICATE-ENABLED expects YES or NO");
    }
#endif // CYBER_LIB
}

//---------------------------------------------------------------------------
// FUNCTION             : MacAddNewInterface
// PURPOSE             :: Adds new ipv6 interface to the node specified.
// PARAMETERS          ::
// + node               : Node* node : Pointer to node
// + globalAddr         : in6_addr* globalAddr  : IPv6 Address pointer,
//                          the global address to associate with the interface.
// + tla                : unsigned int tla: Top level aggregation
// + nla                : unsigned int nla: Next level aggregation
// + sla                : unsigned int sla: Site level aggregation
// + interfaceIndex     : int* interfaceIndex: Interface Index
// + nodeInput          : const NodeInput* nodeInput: Pointer to node input.
// RETURN               : None
// NOTE                 : Overload function to support Ipv6
//---------------------------------------------------------------------------
static void
MacAddNewInterface(
        Node* node,
        in6_addr* globalAddr,
        in6_addr* subnetAddr,
        unsigned int subnetPrefixLen,
        int* interfaceIndex,
        const NodeInput *nodeInput,
        unsigned short siteCounter,
        const char* macProtocolName,
        BOOL isNewInterface)
{
    Address addr;
    addr.networkType = NETWORK_IPV6;
    COPY_ADDR6(*globalAddr, addr.interfaceAddr.ipv6);

    Ipv6AddNewInterface(
        node,
        globalAddr,
        subnetAddr,
        subnetPrefixLen,
        interfaceIndex,
        nodeInput,
        siteCounter,
        isNewInterface);

    if (isNewInterface)
    {
        MacSetInterface(node, *interfaceIndex, &addr, nodeInput);

        MacConfigureHWAddress(node, node->nodeId, *interfaceIndex, nodeInput,
                            &node->macData[*interfaceIndex]->macHWAddr,
                          macProtocolName, NETWORK_IPV6, addr);
    }
}


#ifdef ENTERPRISE_LIB
// --------------------------------------------------------------------------
// FUNCTION: MacAddVlanInfoForThisInterface
// PURPOSE:  Init and read VLAN configuration from user input.
// --------------------------------------------------------------------------
void MacAddVlanInfoForThisInterface(
    Node* node,
    int interfaceIndex,
    Address* address,
    const NodeInput *nodeInput)
{
    BOOL retVal;
    int intInput;
    char buf[MAX_STRING_LENGTH];
    char errString[MAX_STRING_LENGTH];

    // [end station address list] SWITCH-STATION-VLAN-ID <number>
    IO_ReadInt(
        ANY_NODEID,
        address,
        nodeInput,
        "SWITCH-STATION-VLAN-ID",
        &retVal,
        &intInput);

    if (retVal)
    {
        // Allocate required space for vlan
        node->macData[interfaceIndex]->vlan =
            (MacVlan *) MEM_malloc(sizeof(MacVlan));
        memset(node->macData[interfaceIndex]->vlan, 0, sizeof(MacVlan));


        if (intInput >= SWITCH_VLAN_ID_MIN
            && intInput <= SWITCH_VLAN_ID_MAX)
        {
            node->macData[interfaceIndex]->vlan->vlanId =
                (VlanId) intInput;
        }
        else if (intInput == SWITCH_VLAN_ID_NULL)
        {
            node->macData[interfaceIndex]->vlan->sendTagged = FALSE;
            return;
        }
        else
        {
                sprintf(errString,
                    "MacAddVlanInfoForThisInterface: "
                    "Error in SWITCH-STATION-VLAN-ID specification.\n"
                    "Vlan IDs can range from %u to %u\n",
                    SWITCH_VLAN_ID_MIN,
                    SWITCH_VLAN_ID_MAX);
                ERROR_ReportError(errString);
        }

        // [end station address list] SWITCH-STATION-VLAN-TAGGING YES | NO
        node->macData[interfaceIndex]->vlan->sendTagged =
            STATION_VLAN_TAGGING_DEFAULT;

        IO_ReadString(
            ANY_NODEID,
            address,
            nodeInput,
            "SWITCH-STATION-VLAN-TAGGING",
            &retVal,
            buf);

        if (retVal)
        {
            if (!strcmp(buf, "YES"))
            {
                node->macData[interfaceIndex]->vlan->sendTagged = TRUE;
            }
            else if (!strcmp(buf, "NO"))
            {
                node->macData[interfaceIndex]->vlan->sendTagged = FALSE;
            }
            else
            {
                ERROR_ReportError(
                    "MacAddVlanInfoForThisInterface: "
                    "Error in SWITCH-STATION-VLAN-TAGGING user input.\n"
                    "Expecting YES or NO\n");
            }
        }
    }
}


// --------------------------------------------------------------------------
// FUNCTION: MacReleaseVlanInfoForThisInterface
// PURPOSE:  Free VLAN related structure.
// --------------------------------------------------------------------------
void MacReleaseVlanInfoForThisInterface(
    Node* node,
    int interfaceIndex)
{
    if (node->macData[interfaceIndex]->vlan != NULL)
    {
        MEM_free(node->macData[interfaceIndex]->vlan);
    }
}
#endif // ENTERPRISE_LIB


static void
SetMacConfigParameter(
    Node* node,
    int interfaceIndex,
    const NodeInput* nodeInput)
{
    BOOL wasFound;
    char buf[MAX_STRING_LENGTH];
    char yesOrNo[MAX_STRING_LENGTH];

    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "MAC-LAYER-STATISTICS",
        &wasFound,
        yesOrNo);

    if (!wasFound || strcmp(yesOrNo, "NO") == 0)
    {
        node->macData[interfaceIndex]->macStats = FALSE;
    }
    else
    if (strcmp(yesOrNo, "YES") == 0)
    {
        node->macData[interfaceIndex]->macStats = TRUE;
    }
    else
    {
        sprintf(buf,
                "Expecting YES or NO"
                " for MAC-LAYER-STATISTICS parameter\n");
        ERROR_ReportError(buf);
    }
    node->macData[interfaceIndex]->promiscuousModeWithHeaderSwitch = FALSE;
    node->macData[interfaceIndex]->promiscuousModeWithHLA = FALSE;

    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PROMISCUOUS-MODE",
        &wasFound,
        yesOrNo);

    if (!wasFound || strcmp(yesOrNo, "NO") == 0)
    {
        node->macData[interfaceIndex]->promiscuousMode = FALSE;
    }
    else
    if (strcmp(yesOrNo, "YES") == 0)
    {
        node->macData[interfaceIndex]->promiscuousMode = TRUE;
    }
    else
    {
        sprintf(buf,
                "Expecting YES or NO"
                " for PROMISCUOUS-MODE parameter\n");
        ERROR_ReportError(buf);
    }

    node->macData[interfaceIndex]->propDelay = MAC_PROPAGATION_DELAY;

    IO_ReadTime(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "MAC-PROPAGATION-DELAY",
        &wasFound,
        &node->macData[interfaceIndex]->propDelay);

    node->macData[interfaceIndex]->phyNumber = -1;
    node->macData[interfaceIndex]->bandwidth = -1;

    for (int k = 0; k < MAX_PHYS_PER_MAC; k++)
    {
      node->macData[interfaceIndex]->phyNumberArray[k] = -1;
    }

    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "LLC-ENABLED",
        &wasFound,
        yesOrNo);

    node->macData[interfaceIndex]->isLLCEnabled = FALSE;
    if (wasFound)
    {
        if (strcmp(yesOrNo, "NO") == 0)
        {
            node->macData[interfaceIndex]->isLLCEnabled = FALSE;
        }
        else
        if (strcmp(yesOrNo, "YES") == 0)
        {
            node->macData[interfaceIndex]->isLLCEnabled = TRUE;
        }
        else
        {
            node->macData[interfaceIndex]->isLLCEnabled = FALSE;
        }
    }
}

//---------------------------------------------------------------------------
// FUNCTION  : CreateMac802_3
// RETURN    : None
// NOTE      : This function is also called from CreateLink(...) for
//             Point to Point Full Duplex Link Initialization.
//             for that reason this default parameter BOOL fromLink is used.
//---------------------------------------------------------------------------

static void
CreateMac802_3(
    Node* node,
    Address* address,
    const NodeInput* nodeInput,
    int interfaceIndex,
    SubnetMemberData* subnetList,
    int nodesInSubnet,
    NetworkType networkType,
    BOOL fromLink = FALSE)
{
    Int64 subnetBandwidth;
    clocktype subnetPropDelay;

#ifdef PARALLEL //Parallel
    int n;

    if (fromLink == FALSE
        && subnetList[0].node != NULL
        && node->nodeId == subnetList[0].node->nodeId
        && node->partitionId == node->partitionData->partitionId) {
        for (n = 1; n < nodesInSubnet; n++) {
            if (subnetList[n].node == NULL
                || subnetList[n].node->partitionId
                   != subnetList[0].node->partitionId) {
                ERROR_ReportError
                    ("802.3 networks cannot be split across partitions.");
            }
        }
    }
#endif //endParallel

    Mac802_3GetSubnetParameters(node,
                                interfaceIndex,
                                nodeInput,
                                address,
                                &subnetBandwidth,
                                &subnetPropDelay,
                                fromLink);

    if (fromLink)
    {
        SetMacConfigParameter(node,
                              interfaceIndex,
                              nodeInput);
    }

    NetworkIpCreateQueues(node, nodeInput, interfaceIndex);

    node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_802_3;
    node->macData[interfaceIndex]->bandwidth = subnetBandwidth;
    node->macData[interfaceIndex]->propDelay = subnetPropDelay;
    node->macData[interfaceIndex]->mplsVar = NULL;

    Mac802_3Init(node,
                 nodeInput,
                 interfaceIndex,
                 subnetList,
                 nodesInSubnet,
                 fromLink);
}


static void
CreateLink(Node*            node,
           const NodeInput* nodeInput,
           int              interfaceIndex,
           int              remoteInterfaceIndex,
           NodeId           nodeId1,
           NodeAddress      interfaceAddress1,
           NodeId           nodeId2,
           NodeAddress      interfaceAddress2,
           NodeAddress      subnetAddress,
           int              numHostBits,
           Mac802Address    linkAddr2)
{
    LinkData* link;
    MacData*  thisMac;
    Address   address;
    SubnetMemberData *subnetList;
    char linkMacString[MAX_STRING_LENGTH];
    Node* node1;
    Node* node2;
    BOOL wasFound = FALSE;
    BOOL addDefaultRoute = TRUE;
    NodeAddress      interfaceAddress = 0;
    thisMac = node->macData[interfaceIndex];
    char buf[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;

    if (node->nodeId == nodeId1)
    {
        interfaceAddress = interfaceAddress1;
        SetIPv4AddressInfo(&address, interfaceAddress1);
    }
    else if (node->nodeId == nodeId2)
    {
        interfaceAddress = interfaceAddress2;
        SetIPv4AddressInfo(&address, interfaceAddress2);
    }
    // Check if the link is a Point-to-Point Full Duplex Link
    // Normally Mac doesn't run on link but for Point-to-Point
    // Full Duplex Link it uses Mac protocol 802.3
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "LINK-MAC-PROTOCOL",
        &wasFound,
        linkMacString);

    if ((!wasFound) || (!strcmp(linkMacString, "ABSTRACT")))
    {
        // not found or the link is default point to point
        Mac802Address linkAddr1;
        ConvertVariableHWAddressTo802Address(node,
            &node->macData[interfaceIndex]->macHWAddr, &linkAddr1);

        LinkInit(node,
             nodeInput,
             &address,
             interfaceIndex,
             remoteInterfaceIndex,
             nodeId1,
             linkAddr1,
             nodeId2,
             linkAddr2);

        // check to see if we want to automatically generate a default route
        // default behavior is to generate one...
        IO_ReadBool(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "LINK-GENERATE-AUTOMATIC-DEFAULT-ROUTE",
                &wasFound,
                &addDefaultRoute);

        // Check if destination node is not switch
        LinkData* link = (LinkData*)node->macData[interfaceIndex]->macVar;
        IO_ReadString(
            link->destNodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH",
            &retVal,
            buf);
        if (retVal && !strcmp(buf, "YES"))
        {
            link->isPointToPointLink = FALSE;
        }
        else
        {
            link->isPointToPointLink = TRUE;
        }
    }
    else if (!strcmp(linkMacString, "MAC802.3"))
    {
        subnetList = (SubnetMemberData*)
                     MEM_malloc(sizeof(SubnetMemberData) * 2);

        int pIndex;
        pIndex = PARALLEL_GetPartitionForNode(nodeId1);

        if (pIndex == node->partitionData->partitionId) {
            node1 = MAPPING_GetNodePtrFromHash(node->partitionData->nodeIdHash,
                                                nodeId1);
        }
        else {
            node1 = NULL;
        }

        // Fill out the subnetList structure
        subnetList[0].node = node1;
        subnetList[0].nodeId = nodeId1;
        subnetList[0].address.networkType = NETWORK_IPV4;
        subnetList[0].address.interfaceAddr.ipv4 = interfaceAddress1;
        subnetList[0].partitionIndex = pIndex;

        pIndex = PARALLEL_GetPartitionForNode(nodeId2);

        if (pIndex == node->partitionData->partitionId) {
            // In same partition
            node2 = MAPPING_GetNodePtrFromHash(node->partitionData->nodeIdHash,
                     nodeId2);
        }
        else {
            node2 = NULL;
        }

        // Fill out the subnetList structure
        subnetList[1].node = node2;
        subnetList[1].nodeId = nodeId2;
        subnetList[1].address.networkType = NETWORK_IPV4;
        subnetList[1].address.interfaceAddr.ipv4 = interfaceAddress2;
        subnetList[1].partitionIndex = pIndex;

        if (nodeId1 == node->nodeId)
        {
            subnetList[0].interfaceIndex = interfaceIndex;
            subnetList[1].interfaceIndex = remoteInterfaceIndex;
        }
        else
        {
            subnetList[0].interfaceIndex = remoteInterfaceIndex;
            subnetList[1].interfaceIndex = interfaceIndex;
        }

        CreateMac802_3(node,
                &address,
                nodeInput,
                interfaceIndex,
                subnetList,
                2,
                NETWORK_IPV4,
                TRUE);

        // Check if destination node is not switch
        MacData802_3* mac802_3 = (MacData802_3*)node->macData[interfaceIndex]->macVar;
        IO_ReadString(
            mac802_3->link->destNodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH",
            &retVal,
            buf);
        if (retVal && !strcmp(buf, "YES"))
        {
            mac802_3->link->isPointToPointLink = FALSE;
        }
        else
        {
            mac802_3->link->isPointToPointLink = TRUE;
            if (!mac802_3->isFullDuplex)
            {
                char errorStr[MAX_STRING_LENGTH];
                sprintf(errorStr, "Point to Point Link between node %d & %d must "
                          "be FULL-DUPLEX", node->nodeId, mac802_3->link->destNodeId);
                ERROR_ReportError(errorStr);
            }
        }
    }
#ifdef ADDON_BOEINGFCS
    else if (!strcmp(linkMacString, "EPLRS-LINK")
        || (!strcmp(linkMacString, "SINCGARS-LINK")))
    {
        char protocolString[MAX_STRING_LENGTH] = {0};
        IO_ReadStringInstance(
            node->nodeId,
            &address,
            nodeInput,
            "NETWORK-PROTOCOL",
            interfaceIndex,
            TRUE,
            &wasFound,
            protocolString);

        if (!strcmp(linkMacString, "EPLRS-LINK") &&
            (!strcmp(protocolString, "IP")))
        {
            node->macData[interfaceIndex]->macProtocol =
                MAC_PROTOCOL_CES_EPLRS;
            MacCesEplrsInit(node, interfaceIndex, interfaceAddress,
                nodeInput, 0);
        }

        // not found or the link is default point to point
        Mac802Address linkAddr1;
        ConvertVariableHWAddressTo802Address(node,
            &node->macData[interfaceIndex]->macHWAddr, &linkAddr1);

        LinkInit(node,
            nodeInput,
            &address,
            interfaceIndex,
            remoteInterfaceIndex,
            nodeId1,
            linkAddr1,
            nodeId2,
            linkAddr2);

        if (!strcmp(linkMacString, "EPLRS-LINK") &&
            !(strcmp(protocolString, "IP") == 0))
        {
            node->macData[interfaceIndex]->macProtocol =
                MAC_PROTOCOL_LINK_EPLRS;//MAC_PROTOCOL_CES_EPLRS_LINK
        }
    }
#endif
    // only add default route for 802.3 or p2p link connected to a switch.
    if (addDefaultRoute)
    {
        /* Automatically add directly connected route to forwarding table. */
        NetworkUpdateForwardingTable(
            node,
            subnetAddress,
            ConvertNumHostBitsToSubnetMask(numHostBits),
            0,
            interfaceIndex,
            1,
            ROUTING_PROTOCOL_DEFAULT);
    }

    if (node->guiOption && node->nodeId == nodeId1)
    {
        link = (LinkData *)thisMac->macVar;

        if (link->phyType == WIRELESS || link->phyType == SATELLITE) {
            GUI_AddLink(nodeId1, nodeId2,
                        (GuiLayers) MAC_LAYER, GUI_WIRELESS_LINK_TYPE,
                        subnetAddress, numHostBits,
                        node->getNodeTime() + getSimStartTime(node));
        }
        else {
            GUI_AddLink(nodeId1, nodeId2,
                        (GuiLayers) MAC_LAYER, GUI_DEFAULT_LINK_TYPE,
                        subnetAddress, numHostBits,
                        node->getNodeTime() + getSimStartTime(node));
        }
    }
}
// --------------------------------------------------------------------------
// FUNCTION: CreateIpv6Link
// PURPOSE:  Add a node to a subnet.
// PARAMETERS:
// + node           : Node* node : Pointer to the Node
// + nodeInput      : const NodeInput* nodeInput: pointer to node input
// + interfaceIndex : int interfaceIndex: Index of concerned interface
// + nodeId1        : NodeId nodeId1: First node id of the link
// + interfaceAddress1 : in6_addr* interfaceAddress1: IPv6 address pointer,
//                         first interface's v6 address of the link.
// + nodeId2        : NodeId nodeId2: Second node id of the link.
// + interfaceAddress2 : in6_addr* interfaceAddres2: IPv6 address pointer,
//                         second interface's v6 address of the link.
// + linkLayerAddr1 : NodeAddress linkLayerAddr1: Link layer address,
//                          first interface's link layer address.
// + linkLayerAddr2 : NodeAddress linkLayerAddr2: Link layer address,
//                          Second interface's link layer address.
// + tla            : unsigned int tla: Top level aggregation.
// + nla            : unsigned int nla: Next level aggregation.
// + sla            : unsigned int sla: Site level aggregation.
// RETURN : None
// --------------------------------------------------------------------------


static void
CreateIpv6Link(Node* node, const NodeInput* nodeInput,
               int interfaceIndex, int remoteInterfaceIndex,
               NodeId nodeId1, in6_addr* interfaceAddress1,
               NodeId nodeId2, in6_addr* interfaceAddress2,
               Mac802Address linkLayerAddr1, Mac802Address linkLayerAddr2,
               in6_addr IPv6subnetAddress,
               unsigned int IPv6subnetPrefixLen)
{
    LinkData* link;
    MacData*  thisMac;
    Address   address;
    SubnetMemberData *subnetList;
    char linkMacString[MAX_STRING_LENGTH];
    Node* node1;
    Node* node2;
    BOOL wasFound = FALSE;
    thisMac = node->macData[interfaceIndex];
    char buf[MAX_STRING_LENGTH];
    BOOL retVal = FALSE;

    if (node->nodeId == nodeId1)
    {
        SetIPv6AddressInfo(&address, *interfaceAddress1);
    }
    else if (node->nodeId == nodeId2)
    {
        SetIPv6AddressInfo(&address, *interfaceAddress2);
    }

    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "LINK-MAC-PROTOCOL",
        &wasFound,
        linkMacString);

    if ((!wasFound) || (!strcmp(linkMacString, "ABSTRACT")))
    {
        // not found or the link is default point to point
        LinkInit(
            node,
            nodeInput,
            &address,
            interfaceIndex,
            remoteInterfaceIndex,
            nodeId1,
            linkLayerAddr1,
            nodeId2,
            linkLayerAddr2);

        // Check if destination node is not switch
        LinkData* link = (LinkData*)node->macData[interfaceIndex]->macVar;
        IO_ReadString(
            link->destNodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH",
            &retVal,
            buf);
        if (retVal && !strcmp(buf, "YES"))
        {
            link->isPointToPointLink = FALSE;
        }
        else
        {
            link->isPointToPointLink = TRUE;
        }
    }
    else if (!strcmp(linkMacString, "MAC802.3"))
    {
        subnetList = (SubnetMemberData*)
                     MEM_malloc(sizeof(SubnetMemberData) * 2);

        int pIndex;
        pIndex = PARALLEL_GetPartitionForNode(nodeId1);

        if (pIndex == node->partitionData->partitionId) {
            node1 = MAPPING_GetNodePtrFromHash(node->partitionData->nodeIdHash,
                                                nodeId1);
        }
        else {
            node1 = NULL;
        }

        // Fill out the subnetList structure
        subnetList[0].node = node1;
        subnetList[0].nodeId = nodeId1;
        subnetList[0].address.networkType = NETWORK_IPV6;
        subnetList[0].partitionIndex = pIndex;

        pIndex = PARALLEL_GetPartitionForNode(nodeId2);

        if (pIndex == node->partitionData->partitionId) {
            // In same partition
            node2 = MAPPING_GetNodePtrFromHash(node->partitionData->nodeIdHash,
                     nodeId2);
        }
        else {
            node2 = NULL;
        }

        // Fill out the subnetList structure
        subnetList[1].node = node2;
        subnetList[1].nodeId = nodeId2;
        subnetList[1].address.networkType = NETWORK_IPV6;
        subnetList[1].partitionIndex = pIndex;

        if (nodeId1 == node->nodeId)
        {
            subnetList[0].interfaceIndex = interfaceIndex;
            subnetList[1].interfaceIndex = remoteInterfaceIndex;
        }
        else
        {
            subnetList[0].interfaceIndex = remoteInterfaceIndex;
            subnetList[1].interfaceIndex = interfaceIndex;
        }

        CreateMac802_3(node,
                &address,
                nodeInput,
                interfaceIndex,
                subnetList,
                2,
                NETWORK_IPV6,
                TRUE);

        // Check if destination node is not switch
        MacData802_3* mac802_3 = (MacData802_3*)node->macData[interfaceIndex]->macVar;
        IO_ReadString(
            mac802_3->link->destNodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH",
            &retVal,
            buf);
        if (retVal && !strcmp(buf, "YES"))
        {
            mac802_3->link->isPointToPointLink = FALSE;
        }
        else
        {
            mac802_3->link->isPointToPointLink = TRUE;
            if (!mac802_3->isFullDuplex)
            {
                char errorStr[MAX_STRING_LENGTH];
                sprintf(errorStr, "Point to Point Link between node %d & %d must "
                          "be FULL-DUPLEX", node->nodeId, mac802_3->link->destNodeId);
                ERROR_ReportError(errorStr);
            }
        }
    }

    link = (LinkData *)thisMac->macVar;

    if (node->guiOption && node->nodeId == nodeId1)
    {
        if (link->phyType == WIRELESS)
        {
            // IPV6_Start
            // Pass address and prefix length instead of TLA, NLA, SLA
            GUI_AddLink(nodeId1, nodeId2,
                        (GuiLayers) MAC_LAYER, GUI_WIRELESS_LINK_TYPE,
                        (char*)&IPv6subnetAddress,
                        IPv6subnetPrefixLen,
                        node->getNodeTime() + getSimStartTime(node));
        }
        else
        {
            GUI_AddLink(nodeId1, nodeId2,
                        (GuiLayers) MAC_LAYER, GUI_DEFAULT_LINK_TYPE,
                        (char*)&IPv6subnetAddress,
                        IPv6subnetPrefixLen,
                        node->getNodeTime() + getSimStartTime(node));
        }
    }
}


#ifdef ENTERPRISE_LIB
static void
CreateSwitchedEthernet(
    Node* node,
    const Address* address,
    const NodeInput* nodeInput,
    int interfaceIndex,
    SubnetMemberData* subnetList,
    int nodesInSubnet)
{
    Int64 subnetBandwidth;
    clocktype subnetPropDelay;

#ifdef PARALLEL //Parallel
    int n;

    if (subnetList[0].node != NULL
        && node->nodeId == subnetList[0].node->nodeId
        && node->partitionId == node->partitionData->partitionId) {
        for (n = 1; n < nodesInSubnet; n++) {
            if (subnetList[n].node == NULL
                || subnetList[n].node->partitionId
                   != subnetList[0].node->partitionId) {
                ERROR_ReportError
                    ("Switched Ethernet networks cannot be split "
                     "across partitions.");
            }
        }
    }
#endif //endParallel

    ReturnSwitchedEthernetSubnetParameters(
        node,
        interfaceIndex,
        address,
        nodeInput,
        &subnetBandwidth,
        &subnetPropDelay);

    NetworkIpCreateQueues(node, nodeInput, interfaceIndex);

    node->macData[interfaceIndex]->macProtocol =
        MAC_PROTOCOL_SWITCHED_ETHERNET;

    node->macData[interfaceIndex]->bandwidth = subnetBandwidth;
    node->macData[interfaceIndex]->propDelay = subnetPropDelay;

    SwitchedEthernetInit(
        node,
        nodeInput,
        interfaceIndex,
        subnetList,
        nodesInSubnet);

    node->macData[interfaceIndex]->mplsVar = NULL;
}
#endif // ENTERPRISE_LIB


static BOOL
CreateSatcom(
    Node* node,
    Address* address,
    const NodeInput* nodeInput,
    int interfaceIndex,
    int nodesInSubnet,
    const int subnetListIndex,
    SubnetMemberData* subnetList)
{
    Int64 bandwidth;
    clocktype uplinkDelay;
    clocktype downlinkDelay;
    BOOL userSpecifiedPropDelay;


/*
#ifdef PARALLEL //Parallel
    int n;

    if (subnetList[0].node != NULL
        && node->nodeId == subnetList[0].node->nodeId
        && node->partitionId == node->partitionData->partitionId) {
        for (n = 1; n < nodesInSubnet; n++) {
            if (subnetList[n].node == NULL
                || subnetList[n].node->partitionId
                   != subnetList[0].node->partitionId) {
                ERROR_ReportError
                    ("SATCOM networks cannot be split across partitions.");
            }
        }
    }
#endif //endParallel
i*/
    MacSatComGetSubnetParameters(
        node,
        interfaceIndex,
        address,
        nodeInput,
        &bandwidth,
        &userSpecifiedPropDelay,
        &uplinkDelay,
        &downlinkDelay);

    NetworkIpCreateQueues(node, nodeInput, interfaceIndex);

    node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_SATCOM;
    node->macData[interfaceIndex]->bandwidth = bandwidth;
    node->macData[interfaceIndex]->propDelay = uplinkDelay + downlinkDelay;

    node->macData[interfaceIndex]->mplsVar = NULL;

    if (MacSatComInit(
        node,
        nodeInput,
        interfaceIndex,
        nodesInSubnet,
        userSpecifiedPropDelay,
        uplinkDelay,
        downlinkDelay,
        subnetListIndex,
        subnetList))
    {
        return TRUE;
    }

    return FALSE;
}

#ifdef ADDON_BOEINGFCS
static BOOL
CreateWintSatcom(
    Node* node,
    Address* address,
    const NodeInput* nodeInput,
    int interfaceIndex,
    int nodesInSubnet,
    const int subnetListIndex)
{
    Int64 bandwidth;
    clocktype uplinkDelay;
    clocktype downlinkDelay;
    BOOL userSpecifiedPropDelay;

    MacCesWintNcwGetSubnetParameters(
        address,
        nodeInput,
        &bandwidth,
        &userSpecifiedPropDelay,
        &uplinkDelay,
        &downlinkDelay);

    NetworkIpCreateQueues(node, nodeInput, interfaceIndex);

    node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CES_WINTNCW;
    node->macData[interfaceIndex]->bandwidth = bandwidth;
    node->macData[interfaceIndex]->propDelay = uplinkDelay + downlinkDelay;

    node->macData[interfaceIndex]->mplsVar = NULL;

    if (MacCesWintNcwInit(
        node,
        nodeInput,
        interfaceIndex,
        address,
        nodesInSubnet,
        userSpecifiedPropDelay,
        uplinkDelay,
        downlinkDelay,
        subnetListIndex))
    {
        return TRUE;
    }

    return FALSE;
}


static BOOL
CreateWintGBS(
    Node* node,
    Address* address,
    const NodeInput* nodeInput,
    int interfaceIndex,
    int nodesInSubnet)
{
    Int64 bandwidth;
    clocktype uplinkDelay;
    clocktype downlinkDelay;
    BOOL userSpecifiedPropDelay;

    MacCesWintGBSGetSubnetParameters(
        address,
        nodeInput,
        &bandwidth,
        &userSpecifiedPropDelay,
        &uplinkDelay,
        &downlinkDelay);

    NetworkIpCreateQueues(node, nodeInput, interfaceIndex);

    node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CES_WINTGBS;
    node->macData[interfaceIndex]->bandwidth = bandwidth;
    node->macData[interfaceIndex]->propDelay = uplinkDelay + downlinkDelay;

    node->macData[interfaceIndex]->mplsVar = NULL;

    if (MacCesWintGBSInit(
        node,
        nodeInput,
        interfaceIndex,
        address,
        nodesInSubnet,
        userSpecifiedPropDelay,
        uplinkDelay,
        downlinkDelay))
    {
        return TRUE;
    }

    return FALSE;
}
#endif

// --------------------------------------------------------------------------
// FUNCTION: AddNodeToSubnet
// PURPOSE:  Add a node to a subnet.
// --------------------------------------------------------------------------
static void //inline//
AddNodeToSubnet(
    Node *node,
    const NodeInput *nodeInput,
    NodeAddress subnetAddress,    //base address
    NodeAddress interfaceAddress, //my IP# on this interface
    int numHostBits,
    char *macProtocolName,
    SubnetMemberData* subnetList,
    int         nodesInSubnet,
    int         subnetListIndex,
    BOOL        isNewInterface,
    short       subnetIndex)
{
    int interfaceIndex;
    BOOL phyModelFound;
    char phyModelName[MAX_STRING_LENGTH];
    Address address;

    MacAddNewInterface(
        node,
        interfaceAddress,
        numHostBits,
        &interfaceIndex,
        nodeInput,
        macProtocolName,
        isNewInterface);

    if (strcmp(macProtocolName, "MAC802.3") == 0
        ||strcmp(macProtocolName, "SATCOM") == 0
#ifdef ENTERPRISE_LIB
        ||strcmp(macProtocolName, "SWITCHED-ETHERNET") == 0
#endif //ENTERPRISE_LIB
        )
    {
        /* Automatically add directly connected route to forwarding table. */
        NetworkUpdateForwardingTable(
                                node,
                                subnetAddress,
                                ConvertNumHostBitsToSubnetMask(numHostBits),
                                0,
                                interfaceIndex,
                                1,
                                ROUTING_PROTOCOL_DEFAULT);
    }

    if (!isNewInterface)
    {
        return;
    }

    SetIPv4AddressInfo(&address, interfaceAddress);

#ifdef ENTERPRISE_LIB
    MacAddVlanInfoForThisInterface(node,
                                   interfaceIndex,
                                   &address,
                                   nodeInput);
#endif // ENTERPRISE_LIB

    subnetList[subnetListIndex].interfaceIndex = interfaceIndex;
    node->macData[interfaceIndex]->subnetIndex = subnetIndex;

    SetMacConfigParameter(node,
                          interfaceIndex,
                          nodeInput);

#ifdef LTE_LIB
    // if this I/F is added to EPC-SUBNET, initialize EpcData
    {
        NodeAddress na = NetworkIpGetInterfaceAddress(node, interfaceIndex);
        BOOL epcWasFound = FALSE;
        BOOL isEpcSubnet = FALSE;
        IO_ReadBool(
            node->nodeId,
            na,
            nodeInput,
            "IS-EPC-SUBNET",
            &epcWasFound,
            &isEpcSubnet);
        if (epcWasFound && isEpcSubnet)
        {
            // node id
            int tempSgwmmeNodeId;
            NodeAddress sgwmmeNodeId = ANY_NODEID;
            IO_ReadInt(
                node->nodeId,
                na,
                nodeInput,
                "EPC-SGWMME-NODE-ID",
                &epcWasFound,
                &tempSgwmmeNodeId);
            if (epcWasFound) sgwmmeNodeId = tempSgwmmeNodeId;
            // interfaceIndex
            int sgwmmeInterfaceIndex = NETWORK_UNREACHABLE;
            IO_ReadInt(
                node->nodeId,
                na,
                nodeInput,
                "EPC-SGWMME-INTERFACE-INDEX",
                &epcWasFound,
                &sgwmmeInterfaceIndex);
            EpcLteInit(node, interfaceIndex,
                sgwmmeNodeId, sgwmmeInterfaceIndex);
        }
    }
#endif // LTE_LIB

    /* Select the MAC protocol and initialize. */

    if (strcmp(macProtocolName, "MAC802.3") == 0)
    {
        if (nodesInSubnet < 2)
        {
            char errorStr[MAX_STRING_LENGTH];
            char subnetAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(subnetAddress, subnetAddr);

            sprintf(errorStr, "There are %d nodes (fewer than two) in subnet "
                              "%s", nodesInSubnet, subnetAddr);
            ERROR_ReportError(errorStr);
        }

        CreateMac802_3(node,
                       &address,
                       nodeInput,
                       interfaceIndex,
                       (SubnetMemberData *) subnetList,
                       nodesInSubnet,
                       NETWORK_IPV4);

        return;
    }
    else if (strcmp(macProtocolName, "SATCOM") == 0)
    {
        if (nodesInSubnet < 3)
        {
            char errorStr[MAX_STRING_LENGTH];

            char subnetAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(subnetAddress, subnetAddr);
            sprintf(errorStr, "There are %d nodes (fewer than three) in "
                    "SAT-COM subnet "
                    "%s", nodesInSubnet, subnetAddr);

            ERROR_ReportError(errorStr);
        }

        CreateSatcom(node,
                         &address,
                         nodeInput,
                         interfaceIndex,
                         nodesInSubnet,
                         subnetListIndex,
                         (SubnetMemberData *) subnetList);

        return;
    }
    else if (strcmp(macProtocolName, "SWITCHED-ETHERNET") == 0)
    {
#ifdef ENTERPRISE_LIB
        // Don't create a physical model for switched ethernet
        // because the switched ethernet model is an abstract model
        // We do need to acquire subnet-specific parameters for
        // bandwidth and prop delay, though
        if (nodesInSubnet < 2)
        {
            char errorStr[MAX_STRING_LENGTH];
            char subnetAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(subnetAddress, subnetAddr);


            sprintf(errorStr, "There are %d nodes (fewer than two) in subnet "
                    "%s", nodesInSubnet, subnetAddr);

            ERROR_ReportError(errorStr);
        }

        CreateSwitchedEthernet(node,
                               &address,
                               nodeInput,
                               interfaceIndex,
                               (SubnetMemberData *) subnetList,
                               nodesInSubnet);

        return;
#else //ENTERPRISE_LIB
        ERROR_ReportMissingLibrary(macProtocolName, "ENTERPRISE_LIB");
#endif // ENTERPRISE_LIB
    }
#ifdef ADDON_BOEINGFCS
//JW
    else if (!strcmp(macProtocolName, "MAC-CES-SRW"))
    {
        node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CES_SRW_PORT;

        if (SRW::Nexus::CreateInterface(node,
                                        &address,
                                        nodeInput,
                                        interfaceIndex,
                                        nodesInSubnet,
                                        subnetListIndex,
                                        (SubnetMemberData*) subnetList))
        {
          NetworkUpdateForwardingTable(node,
                                       subnetAddress,
                                       ConvertNumHostBitsToSubnetMask(numHostBits),
                                       0,
                                       interfaceIndex,
                                       1,
                                       ROUTING_PROTOCOL_DEFAULT);

          SRW::Nexus::InitializeInterface(node, interfaceIndex,
                                          interfaceAddress, nodeInput,
                                          nodesInSubnet, subnetListIndex);
        }

        return;
    }
    else if (strcmp(macProtocolName, "MAC-CES-WIN-T-NCW") == 0)
    {
        if (nodesInSubnet < 3)
        {
            char errorStr[MAX_STRING_LENGTH];
            char subnetAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(subnetAddress, subnetAddr);


            sprintf(errorStr, "There are %d nodes (fewer than three) in "
                    "MAC-CES-WIN-T-NCW subnet "
                    "%s", nodesInSubnet, subnetAddr);

            ERROR_ReportError(errorStr);
        }

        if (CreateWintSatcom(node,
                         &address,
                         nodeInput,
                         interfaceIndex,
                         nodesInSubnet,
                         subnetListIndex))
        {
            NetworkUpdateForwardingTable(
                    node,
                    subnetAddress,
                    ConvertNumHostBitsToSubnetMask(numHostBits),
                    0,
                    interfaceIndex,
                    1,
                    ROUTING_PROTOCOL_DEFAULT);
        }
        return;
    }
    else if (strcmp(macProtocolName, "MAC-CES-WIN-T-GBS") == 0)
    {
        if (nodesInSubnet < 3)
        {
            char errorStr[MAX_STRING_LENGTH];
            char subnetAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(subnetAddress, subnetAddr);



            sprintf(errorStr, "There are %d nodes (fewer than three) in "
                    "CES-WIN-T-GBS subnet "
                            "%s", nodesInSubnet, subnetAddr);

            ERROR_ReportError(errorStr);
        }

        if (CreateWintGBS(node,
            &address,
            nodeInput,
            interfaceIndex,
            nodesInSubnet))
        {
            NetworkUpdateForwardingTable(
                    node,
            subnetAddress,
            ConvertNumHostBitsToSubnetMask(numHostBits),
            0,
            interfaceIndex,
            1,
            ROUTING_PROTOCOL_DEFAULT);
        }
        return;
    }
#endif /* ADDON_BOEINGFCS */
#ifdef MODE5_LIB
    else if (strcmp(macProtocolName, "MODE5") == 0)
    {
        NetworkIpCreateQueues(node, nodeInput, interfaceIndex);
        node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_MODE5;
        node->macData[interfaceIndex]->macVar = NULL;
    }
#endif
    else if (strcmp(macProtocolName, "ANE") == 0)
    {
#ifdef SATELLITE_LIB
        NetworkIpCreateQueues(node, nodeInput, interfaceIndex);

        node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_ANE;

        node->macData[interfaceIndex]->macVar =
            (void*) new MacAneState(node,
                                    interfaceIndex,
                                    macProtocolName);

        MacAneInitialize(node,
                         interfaceIndex,
                         nodeInput,
                         interfaceAddress,
                         (SubnetMemberData*)subnetList,
                         nodesInSubnet,
                         subnetListIndex);

        if (node->macData[interfaceIndex]->macVar == NULL)
        {
            char errorStr[MAX_STRING_LENGTH];
            char subnetAddr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(subnetAddress, subnetAddr);


            sprintf(errorStr, "Failure to initialized SatTsm Module subnet %s.",
                subnetAddr);
            ERROR_ReportError(errorStr);
        }

        node->macData[interfaceIndex]->mplsVar = NULL;

        return;
#else //SATELLITE_LIB
        ERROR_ReportMissingLibrary(macProtocolName, "SATELLITE_LIB");
#endif // SATELLITE_LIB
    }

    //
    // MAC protocol models below require a PHY model
    //
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-MODEL",
        &phyModelFound,
        phyModelName);

    if (!phyModelFound) {
        char errorMessage[MAX_STRING_LENGTH];
        char addrString[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(interfaceAddress, addrString);
        sprintf(errorMessage,
                "No PHY-MODEL specified for node %u interface %d "
                "with interface address as %s\n",
                node->nodeId, interfaceIndex, addrString);
        ERROR_ReportError(errorMessage);
    }
#ifdef CYBER_LIB
    node->macData[interfaceIndex]->isWormhole = FALSE;
#endif // CYBER_LIB

    if (strncmp(macProtocolName, "FCSC-", 5) == 0) {
#ifdef MILITARY_RADIOS_LIB
        MacFcscInit(
            node,
            nodeInput,
            interfaceIndex,
            &address,
            macProtocolName,
            phyModelName);
#else /* FCSC */
        ERROR_ReportMissingLibrary(macProtocolName, "Military Radios");
#endif // FCSC */
    }
    else {
        PhyModel phyModel = PHY802_11b;

        //
        // Non FCS Comms MAC protocols
        //
        if (strncmp(phyModelName, "FCSC-", 5) == 0) {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage,
                    "%s does not work with %s; use a non-FCSC PHY model\n",
                    macProtocolName, phyModelName);

            ERROR_ReportError(errorMessage);
        }
        else if (strcmp(phyModelName, "PHY802.11a") == 0) {
            int phyNum(-1);

            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY802_11a,
                    &phyNum);

            node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
            phyModel = PHY802_11a;
        }
        else if (strcmp(phyModelName, "PHY802.11b") == 0) {
            int phyNum(-1);

            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY802_11b,
                    &phyNum);

            node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
            phyModel = PHY802_11b;
        }
        else if (strcmp(phyModelName, "PHY802.11n") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY802_11n,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY802_11n;
        }

        else if (strcmp(phyModelName, "PHY-GSM") == 0) {
#ifdef CELLULAR_LIB
            int phyNum(-1);

            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_GSM,
                    &phyNum);

            node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
#else // CELLULAR_LIB
            ERROR_ReportMissingLibrary(phyModelName, "Cellular");
#endif // CELLULAR_LIB
        }
        else if (strcmp(phyModelName, "PHY-CELLULAR") == 0) {
#ifdef WIRELESS_LIB
             PHY_CreateAPhyForMac(
                     node,
                     nodeInput,
                     interfaceIndex,
                     &address,
                     PHY_CELLULAR,
                     &node->macData[interfaceIndex]->phyNumber);
#else
             ERROR_ReportMissingLibrary(phyModelName, "Wireless");
#endif // Wireless
         }
        else if (strcmp(phyModelName, "PHY-ABSTRACT") == 0) {
            int phyNum(-1);

            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_ABSTRACT,
                    &phyNum);

            node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
        }
#ifdef MODE5_LIB
        else if (strcmp(phyModelName, "PHY-MODE5") == 0)
        {
            int phyNum(-1);
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_MODE5,
                    &phyNum);
             node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
        }
#endif
        else if (strcmp(phyModelName, "PHY-SPAWAR-LINK16") == 0)
        {
#ifdef ADDON_LINK16
            int phyNum(-1);

            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_SPAWAR_LINK16,
                    &phyNum);

            node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
#else // ADDON_LINK16
            ERROR_ReportMissingLibrary(phyModelName, "LINK16");
#endif // ADDON_LINK16
        }
        else  if (strcmp(phyModelName, "SATELLITE-RSV") == 0) {
#ifdef SATELLITE_LIB
            int phyNum(-1);

            PHY_CreateAPhyForMac(node, nodeInput,
                                 interfaceIndex, &address,
                                 PHY_SATELLITE_RSV,
                                 &phyNum);

            node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
#else // SATELLITE_LIB
            ERROR_ReportMissingLibrary(phyModelName, "Satellite");
#endif // SATELLITE_LIB

        }
        else  if (strcmp(phyModelName, "PHY802.16") == 0) {
#ifdef ADVANCED_WIRELESS_LIB
            int phyNum(-1);

            PHY_CreateAPhyForMac(node, nodeInput,
                                 interfaceIndex, &address,
                                 PHY802_16,
                                 &phyNum);

            node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
#else // ADVANCED_WIRELESS_LIB
            ERROR_ReportMissingLibrary(phyModelName, "Advanced Wireless");
#endif // ADVANCED_WIRELESS_LIB
        }
        else  if (strcmp(phyModelName, "PHY-LTE") == 0) {
#ifdef LTE_LIB
            LteLayer2PreInit(node, interfaceIndex, nodeInput);
            PHY_CreateAPhyForMac(node, nodeInput,
                                 interfaceIndex, &address,
                                 PHY_LTE,
                                 &node->macData[interfaceIndex]->phyNumber);
#else // LTE_LIB
            ERROR_ReportMissingLibrary(phyModelName, "PHY-LTE");
#endif // LTE_LIB
        }
        else if (strcmp(phyModelName, "PHY-JAMMING") == 0)
        {
#ifdef CYBER_LIB
            int phyNum(-1);

            printf("AddNodeToSubnet(): PHY-JAMMING\n");
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_JAMMING,
                    &phyNum);

            node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
#else // CYBER_LIB
            ERROR_ReportMissingLibrary(phyModelName, "Network Security");
#endif // CYBER_LIB
        }
        else  if (strcmp(phyModelName, "PHY802.15.4") == 0) {
        #ifdef SENSOR_NETWORKS_LIB
                    int phyNum(-1);

                    PHY_CreateAPhyForMac(node, nodeInput,
                                         interfaceIndex, &address,
                                         PHY802_15_4,
                                         &phyNum);

                    node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
        #else // ADVANCED_WIRELESS_LIB
                    ERROR_ReportMissingLibrary(phyModelName, "Sensor Networks");
        #endif // ADVANCED_WIRELESS_LIB
        }
#ifdef ADDON_BOEINGFCS
        else if (strcmp(phyModelName, "PHY-CES-SRW") == 0)
        {
          // decide if we need one PHY or two
          // create them
          // register them

          abort();
        }
#endif
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "Unknown PHY/Radio Model %s configured for "
                    "interface %d of Node %d.\n",
                    phyModelName,
                    interfaceIndex,
                    node->nodeId);
            ERROR_ReportError(errorStr);
        }

#ifdef ADDON_DB
    STATSDB_HandlePhyDescTableInsert(node, interfaceIndex,
        node->macData[interfaceIndex]->phyNumber);
#endif
        //
        // bandwidth is set to the base data rate
        // (in case it's variable)
        //
        node->macData[interfaceIndex]->bandwidth =
            (PHY_GetTxDataRate(
                node,
                node->macData[interfaceIndex]->phyNumber) / 8);

        NetworkIpCreateQueues(node, nodeInput,
                              interfaceIndex);

        if (strcmp(macProtocolName, "CSMA") == 0) {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CSMA;
            MacCsmaInit(node, interfaceIndex, nodeInput);
        }
#ifdef MODE5_LIB
        else if (strcmp(macProtocolName, "MODE5") == 0)
        {
            NodeAddress linkAddress = MAC_VariableHWAddressToFourByteMacAddress(
                                      node,
                                      &node->macData[interfaceIndex]->macHWAddr);

            NetworkIpCreateQueues(node, nodeInput, interfaceIndex);
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_MODE5;
            node->macData[interfaceIndex]->macVar = NULL;
        }
#endif
        else if (strcmp(macProtocolName, "MACA") == 0) {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_MACA;
            MacMacaInit(node, interfaceIndex, nodeInput);
        }
        else if (strcmp(macProtocolName, "TDMA") == 0) {
           node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_TDMA;
           MacTdmaInit(node, interfaceIndex,nodeInput,
                       subnetListIndex, nodesInSubnet);
        }
        else if (strcmp(macProtocolName, "ALOHA") == 0) {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_ALOHA;
            AlohaInit(node, interfaceIndex, nodeInput, nodesInSubnet);
        }
        else if (strcmp(macProtocolName, "GENERICMAC") == 0) {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_GENERICMAC;
            GenericMacInit(node, interfaceIndex,nodeInput, nodesInSubnet);
#ifdef ADDON_BOEINGFCS
        char incString[MAX_STRING_LENGTH];
        BOOL incFound = FALSE;
        IO_ReadString(node->nodeId,
                      interfaceAddress,
                      nodeInput,
                      "INC-TYPE",
                      &incFound,
                      incString);

        if (incFound)
        {
            if (strcmp(incString, "INC-CES-SINCGARS") == 0 ||
                strcmp(incString, "INC-CES-EPLRS") == 0)
            {
                ERROR_ReportError(
                "BOEING-GENERICMAC should be used for BOEINGFCS SINCGARS and EPLRS models!!");
            }
        }
#endif
        }
        else if (strcmp(macProtocolName, "MACDOT11") == 0) {
#ifdef WIRELESS_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_DOT11;
            MacDot11Init(node, interfaceIndex, nodeInput, phyModel,
                         (SubnetMemberData *) subnetList, nodesInSubnet, subnetListIndex,
                         subnetAddress, numHostBits);
#ifdef CYBER_LIB
            BOOL found;
            char buffer[MAX_STRING_LENGTH] = {0};
            IO_ReadString(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "MAC-SECURITY-PROTOCOL",
                &found,
                buffer);

            if (found && strcmp(buffer, "NO") != 0)
            {
                if (strcmp(buffer, "WEP") == 0)
                {
                    WEPInit(node, interfaceIndex, nodeInput, subnetAddress,
                            numHostBits);
                }
                else if (strcmp(buffer, "CCMP") == 0)
                {
                    CCMPInit(node, interfaceIndex, nodeInput, subnetAddress,
                             numHostBits);
                }
                else
                {
                    char errorMessage[MAX_STRING_LENGTH];

                    sprintf(errorMessage,
                            "Invalid MAC security protocol "
                            "specified for node %u, address %u\n",
                            node->nodeId, interfaceAddress);
                    ERROR_ReportError(errorMessage);
                }
            }
#endif // CYBER_LIB
#else // WIRELESS_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Wireless");
#endif // WIRELESS_LIB
        }
        else if (strcmp(macProtocolName, "MACDOT11e") == 0) {
#ifdef WIRELESS_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_DOT11;
            MacDot11Init(node, interfaceIndex, nodeInput, phyModel,
                         (SubnetMemberData *) subnetList, nodesInSubnet, subnetListIndex,
                         subnetAddress, numHostBits,
                         TRUE);
#else // WIRELESS_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Wireless");
#endif // WIRELESS_LIB
        }
        else if (strcmp(macProtocolName, "GSM") == 0) {
#ifdef CELLULAR_LIB
            // Set GSM Parameters By their Default Value
            node->gsmNodeParameters = (GSMNodeParameters *)
            MEM_malloc(sizeof(GSMNodeParameters));
            node->gsmNodeParameters->lac = GSM_INVALID_LAC_IDENTITY;
            node->gsmNodeParameters->cellIdentity = GSM_INVALID_CELL_IDENTITY;

            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_GSM;
            MacGsmInit(node, interfaceIndex, nodeInput, nodesInSubnet);
#else // CELLULAR_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Cellular");
#endif // CELLULAR_LIB
        }
        else if (strcmp(macProtocolName, "ALE") == 0)
        {
#ifdef ALE_ASAPS_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_ALE;
            MacAleInit(node, interfaceIndex, nodeInput, nodesInSubnet);
#else // ALE_ASAPS_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "ALE/ASAPS");
#endif // ALE_ASAPS_LIB
        }
        else if (strcmp(macProtocolName, "MAC-SPAWAR-LINK16") == 0)
        {
#ifdef ADDON_LINK16
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_SPAWAR_LINK16;
            MacSpawar_Link16Init(node, interfaceIndex, interfaceAddress,
                                 nodeInput, nodesInSubnet);
#else // ADDON_LINK16
            ERROR_ReportMissingAddon(macProtocolName, "Link-16");
#endif // ADDON_LINK16
        }

        else if (strcmp(macProtocolName, "MAC-LINK-11") == 0)
        {
#ifdef MILITARY_RADIOS_LIB
            node->macData[interfaceIndex]->macProtocol =
                MAC_PROTOCOL_TADIL_LINK11;
            TadilLink11Init(node, interfaceIndex, nodeInput, nodesInSubnet,
                    subnetListIndex, (SubnetMemberData *)subnetList,
                        numHostBits, subnetAddress);
#else // MILITARY_RADIOS_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Military Radios");
#endif // MILITARY_RADIOS_LIB
        }
        else if (strcmp(macProtocolName, "MAC-LINK-16") == 0)
        {
#ifdef MILITARY_RADIOS_LIB
            node->macData[interfaceIndex]->macProtocol =
                MAC_PROTOCOL_TADIL_LINK16;
            TadilLink16Init(node, interfaceIndex, nodeInput,
                           nodesInSubnet, (SubnetMemberData *)subnetList);
#else // MILITARY_RADIOS_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Military Radios");
#endif // MILITARY_RADIOS_LIB
        }
#ifdef ADDON_BOEINGFCS
        else if (strcmp(macProtocolName, "MAC-CES-WNW-MDL") == 0) {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CES_WNW_MDL;
            MacWnwMiMdlInit(node, interfaceIndex, nodeInput, subnetListIndex, nodesInSubnet);
        }
        else if (strcmp(macProtocolName, "MAC-CES-WIN-T-HNW") == 0) {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CES_WINTHNW;
            MacCesWintHnwInit(node, interfaceIndex, interfaceAddress,
                       nodeInput, (SubnetMemberData *) subnetList, subnetListIndex, nodesInSubnet, subnetIndex);
        }
        else
        if (strcmp(macProtocolName, "MAC-CES-EPLRS") == 0 )
        {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CES_EPLRS;
            MacCesEplrsInit(node, interfaceIndex, interfaceAddress,
                           nodeInput, nodesInSubnet);
        }
        else if (strcmp(macProtocolName, "MAC-CES-SINCGARS") == 0)
        {
                node->macData[interfaceIndex]->macProtocol =
                    MAC_PROTOCOL_CES_SINCGARS;
                MacCesSincgarsInit(node, interfaceIndex, interfaceAddress,
                    nodeInput, nodesInSubnet, subnetListIndex);

        }
        else if (strcmp(macProtocolName, "MAC-CES-SIP") == 0)
        {
                node->macData[interfaceIndex]->macProtocol =
                    MAC_PROTOCOL_CES_SIP;
        }
        else
        if (strcmp(macProtocolName, "MAC-CES-SRW") == 0) {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CES_SRW;
        }
#endif // ADDON_BOEINGFCS

        else if (strcmp(macProtocolName, "SATELLITE-BENTPIPE") == 0) {
#ifdef SATELLITE_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_SATELLITE_BENTPIPE;

            MacSatelliteBentpipeInit(node,
                                     interfaceIndex,
                                     interfaceAddress,
                                     nodeInput,
                                     nodesInSubnet);
#else // SATELLITE_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Satellite");
#endif // SATELLITE_LIB
        }

        else if (strcmp(macProtocolName, "MAC802.16") == 0) {
#ifdef ADVANCED_WIRELESS_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_DOT16;
            MacDot16Init(node, interfaceIndex, nodeInput,
                         macProtocolName, nodesInSubnet, subnetListIndex);
#else // ADVANCED_WIRELESS_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Advanced Wireless");
#endif // ADVANCED_WIRELESS_LIB
        }

        else if (strcmp(macProtocolName, "MAC-LTE") == 0) {
#ifdef LTE_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_LTE;
            LteLayer2Init(node, interfaceIndex, nodeInput);
#else // LTE_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "MAC-LTE");
#endif // LTE_LIB
        }

        else if (strcmp(macProtocolName, "CELLULAR-MAC") == 0)
        {
#ifdef WIRELESS_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CELLULAR;
            MacCellularInit(node,interfaceIndex, nodeInput);
#else
            ERROR_ReportMissingLibrary(macProtocolName, "Wireless");
#endif 
        }
        else if (strcmp(macProtocolName, "MAC-WORMHOLE") == 0)
        {
#ifdef CYBER_LIB
            ERROR_Assert(nodesInSubnet >= 2,
                         "There are less than 2 nodes in a WORMHOLE subnet");
#ifdef PARALLEL //Parallel
            int n;

            if (subnetList[0].node != NULL
                && node->nodeId == subnetList[0].node->nodeId
                && node->partitionId == node->partitionData->partitionId) {
                for (n = 1; n < nodesInSubnet; n++) {
                    if (subnetList[n].node == NULL
                        || subnetList[n].node->partitionId
                        != subnetList[0].node->partitionId) {
                        ERROR_ReportError
                            ("Wormhole networks cannot be split "
                             "across partitions.");
                    }
                }
            }
#endif //endParallel
            MacWormholeInit(node,
                            nodeInput,
                            interfaceIndex,
                            interfaceAddress,
                            (SubnetMemberData *) subnetList,
                            nodesInSubnet,
                            macProtocolName);
#else // CYBER_LIB
            ERROR_ReportMissingLibrary(macProtocolName,
                                       "Network Security");
#endif // CYBER_LIB
        }
        else if (strcmp(macProtocolName, "MAC802.15.4") == 0) {
                if (strcmp(phyModelName, "PHY802.15.4") != 0){
                    char errorMessage[MAX_STRING_LENGTH];
                    sprintf(errorMessage, "Invalid PHY-MODEL %s. "
                            "%s Supports only PHY802.15.4.\n" ,
                            phyModelName, macProtocolName);
                    ERROR_ReportError(errorMessage);
                }

        #ifdef SENSOR_NETWORKS_LIB
                    node->macData[interfaceIndex]->macProtocol =
                                                        MAC_PROTOCOL_802_15_4;
                    Mac802_15_4Init(node, nodeInput, interfaceIndex,
                                    interfaceAddress,
                                    (SubnetMemberData *)subnetList,
                                    nodesInSubnet);
        #else // SENSOR_NETWORKS_LIB
                    ERROR_ReportMissingLibrary(macProtocolName, "Sensor Networks");
        #endif // SENSOR_NETWORKS_LIB
        }
        //InsertPatch MAC_INIT_CODE
        else {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage, "Unknown MAC protocol %s\n",
                    macProtocolName);
            ERROR_ReportError(errorMessage);

        }//if//
        node->macData[interfaceIndex]->mplsVar = NULL;
    }//if//
}

// --------------------------------------------------------------------------
// FUNCTION: AddNodeToIpv6Network
// PURPOSE:  Add a node to a subnet.
// PARAMETERS:
// + node            : Node* node: Pointer to the Node
// + nodeInput       : const NodeInput* nodeInput: Pointer to node input
// + globalAddr      : in6_addr* : IPv6 address pointer,
//                      the global address of the node's interface.
// + tla             : unsigned int tla: Top level aggregation.
// + nla             : unsigned int nla: Next level aggregation.
// + sla             : unsigned int sla: Site level aggregation.
// + macProtocolName : char* macProtocolName: String pointer to mac protocol
//                      name.
// + subnetIndex     : Index of the Subnet
// + subnetList      : SubnetMemberData* subnetList: Pointer to subnet
//                      data list.
// + nodesInSubnet   : int nodesInSubnet: No of nodes in the subnet.
// + subnetListIndex : int subnetListIndex: Index of concerned subnet.
// RETURN : None
// --------------------------------------------------------------------------
static void //inline//
AddNodeToIpv6Network(
    Node *node,
    const NodeInput *nodeInput,
    in6_addr *globalAddr,
    in6_addr *subnetAddr,
    unsigned int subnetPrefixLen,
    char* macProtocolName,
    short subnetIndex,
    SubnetMemberData* subnetList,
    int nodesInSubnet,
    int subnetListIndex,
    unsigned short siteCounter,
    BOOL isNewInterface)
{
    int interfaceIndex;
    BOOL phyModelFound;
    char phyModelName[MAX_STRING_LENGTH];
    NodeAddress linkLayerAddr;
    Address address ;

    MacAddNewInterface(
        node,
        globalAddr,
        subnetAddr,
        subnetPrefixLen,
        &interfaceIndex,
        nodeInput,
        siteCounter,
        macProtocolName,
        isNewInterface);

    if (!isNewInterface)
    {
        return;
    }

    SetIPv6AddressInfo(&address, *globalAddr);

#ifdef ENTERPRISE_LIB
    MacAddVlanInfoForThisInterface(
        node, interfaceIndex, &address, nodeInput);
#endif // ENTERPRISE_LIB

    linkLayerAddr = MAPPING_CreateIpv6LinkLayerAddr(
        node->nodeId, interfaceIndex);

    subnetList[subnetListIndex].interfaceIndex = interfaceIndex;
    node->macData[interfaceIndex]->subnetIndex = subnetIndex;

    SetMacConfigParameter(node,
                          interfaceIndex,
                          nodeInput);

    /* Select the MAC protocol and initialize. */

    if (strcmp(macProtocolName, "MAC802.3") == 0)
    {
        if (nodesInSubnet < 2)
        {
            char errorStr[MAX_STRING_LENGTH];
            UInt32 tla = subnetAddr->s6_addr32[0];
            UInt32 nla = subnetAddr->s6_addr32[1];
            UInt32 sla = subnetAddr->s6_addr32[2];
            sprintf(errorStr, "There are %d nodes (fewer than two) in subnet"
                  " TLA=%u, NLA=%u, SLA=%u.", nodesInSubnet, tla, nla, sla);
            ERROR_ReportError(errorStr);
        }

        CreateMac802_3(node, &address, nodeInput,
            interfaceIndex, subnetList, nodesInSubnet, NETWORK_IPV6);

        return;
    }
    else if (strcmp(macProtocolName, "SATCOM") == 0)
    {
        if (nodesInSubnet < 3)
        {
            char errorStr[MAX_STRING_LENGTH];
            UInt32 tla = subnetAddr->s6_addr32[0];
            UInt32 nla = subnetAddr->s6_addr32[1];
            UInt32 sla = subnetAddr->s6_addr32[2];
            sprintf(errorStr, "There are %d nodes (fewer than three) in "
                    "SAT-COM subnet TLA=%u, NLA=%u, SLA=%u.",
                     nodesInSubnet, tla, nla, sla);

            ERROR_ReportError(errorStr);
        }

        CreateSatcom(node, &address, nodeInput,
            interfaceIndex, nodesInSubnet, subnetListIndex, subnetList);

        return;
    }
    else if (strcmp(macProtocolName, "SWITCHED-ETHERNET") == 0)
    {
#ifdef ENTERPRISE_LIB
        // Don't create a physical model for switched ethernet
        // because the switched ethernet model is an abstract model
        // We do need to acquire subnet-specific parameters for
        // bandwidth and prop delay, though

        if (nodesInSubnet < 2)
        {
            char errorStr[MAX_STRING_LENGTH];
            UInt32 tla = subnetAddr->s6_addr32[0];
            UInt32 nla = subnetAddr->s6_addr32[1];
            UInt32 sla = subnetAddr->s6_addr32[2];
            sprintf(errorStr, "There are %d nodes (fewer than two) in "
                "subnet TLA=%u, NLA=%u, SLA=%u.",
                nodesInSubnet, tla, nla, sla);

            ERROR_ReportError(errorStr);
        }

        CreateSwitchedEthernet(node,
                               &address,
                               nodeInput,
                               interfaceIndex,
                               subnetList,
                               nodesInSubnet);
        return;
#else
        ERROR_ReportMissingLibrary(macProtocolName, "multimedia-enterprise");
#endif // ENTERPRISE_LIB
    }
#ifdef MODE5_LIB
    else if (strcmp(macProtocolName, "MODE5") == 0)
    {
        NodeAddress linkAddress = MAC_VariableHWAddressToFourByteMacAddress(
                                  node,
                                  &node->macData[interfaceIndex]->macHWAddr);

        NetworkIpCreateQueues(node, nodeInput, interfaceIndex);
        node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_MODE5;
        node->macData[interfaceIndex]->macVar = NULL;
    }
#endif
    else if (strcmp(macProtocolName, "ANE") == 0)
    {
#ifdef SATELLITE_LIB
        NetworkIpCreateQueues(node, nodeInput, interfaceIndex);

        node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_ANE;

        NodeAddress linkAddress =
                          MAC_VariableHWAddressToFourByteMacAddress(
                                  node,
                                  &node->macData[interfaceIndex]->macHWAddr);

        node->macData[interfaceIndex]->macVar = (void*)
            (new MacAneState(node,
                             interfaceIndex,
                             macProtocolName));

        MacAneInitialize(node,
                         interfaceIndex,
                         nodeInput,
                         linkAddress,
                         (SubnetMemberData*)subnetList,
                         nodesInSubnet,
                         subnetListIndex);

        if (node->macData[interfaceIndex]->macVar == NULL)
        {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr,
                    "Failure to initialized SatTsm Module subnet %d.",
                    linkAddress);

            ERROR_ReportError(errorStr);
        }

        node->macData[interfaceIndex]->mplsVar = NULL;

        return;
#else
        ERROR_ReportMissingLibrary(macProtocolName, "satellite");
#endif // SATELLITE_LIB
    }

    //
    // MAC protocol models below require a PHY model
    //
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-MODEL",
        &phyModelFound,
        phyModelName);

    if (!phyModelFound) {
        char errorMessage[MAX_STRING_LENGTH];
        char addr[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(globalAddr, addr);
        sprintf(errorMessage,
                "No PHY-MODEL specified for node %u interface %d "
                "with interface address as %s\n",
                node->nodeId, interfaceIndex, addr);
        ERROR_ReportError(errorMessage);
    }

    if (strncmp(macProtocolName, "FCSC-", 5) == 0) {
#ifdef MILITARY_RADIOS_LIB
        MacFcscInit(
            node,
            nodeInput,
            interfaceIndex,
            &address,
            macProtocolName,
            phyModelName);
#else //MILITARY_RADIOS_LIB
        ERROR_ReportMissingLibrary(macProtocolName, "Military Radios");
#endif
    }
    else {
        PhyModel phyModel = PHY802_11b;

        //
        // Non FCS Comms MAC protocols
        //
        if (strncmp(phyModelName, "FCSC-", 5) == 0) {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage,
                    "%s does not work with %s; use a non-FCSC PHY model\n",
                    macProtocolName, phyModelName);

            ERROR_ReportError(errorMessage);
        }
        else if (strcmp(phyModelName, "PHY802.11a") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY802_11a,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY802_11a;
        }
        else if (strcmp(phyModelName, "PHY802.11b") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY802_11b,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY802_11b;
        }
        else if (strcmp(phyModelName, "PHY802.11n") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY802_11n,
                    &node->macData[interfaceIndex]->phyNumber);

            phyModel = PHY802_11n;
        }

        else if (strcmp(phyModelName, "PHY-GSM") == 0) {
#ifdef CELLULAR_LIB
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_GSM,
                    &node->macData[interfaceIndex]->phyNumber);
#else // CELLULAR_LIB
            ERROR_ReportMissingLibrary(phyModelName, "Cellular");
#endif // CELLULAR_LIB
        }

        else if (strcmp(phyModelName, "PHY-CELLULAR") == 0) {
#ifdef WIRELESS_LIB
             PHY_CreateAPhyForMac(
                     node,
                     nodeInput,
                     interfaceIndex,
                     &address,
                     PHY_CELLULAR,
                     &node->macData[interfaceIndex]->phyNumber);
#else
             ERROR_ReportMissingLibrary(phyModelName, "Wireless");
#endif
         }

        else if (strcmp(phyModelName, "PHY-ABSTRACT") == 0) {
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_ABSTRACT,
                    &node->macData[interfaceIndex]->phyNumber);
        }
#ifdef MODE5_LIB
        else if (strcmp(phyModelName, "PHY-MODE5") == 0)
        {
            int phyNum(-1);
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_MODE5,
                    &phyNum);

            node->macData[interfaceIndex]->setPhyNum(phyNum, 0);
        }
#endif

        else if (strcmp(phyModelName, "PHY-SPAWAR-LINK16") == 0)
        {
#ifdef ADDON_LINK16
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_SPAWAR_LINK16,
                    &node->macData[interfaceIndex]->phyNumber);
#else // ADDON_LINK16
            ERROR_ReportMissingAddon(phyModelName, "Link-16");
#endif // ADDON_LINK16
        }
        else if (strcmp(phyModelName, "SATELLITE-RSV") == 0) {
#ifdef SATELLITE_LIB
            PHY_CreateAPhyForMac(node, nodeInput,
                                 interfaceIndex,
                                 &address, PHY_SATELLITE_RSV,
                                 &node->macData[interfaceIndex]->phyNumber);
#else // SATELLITE_LIB
            ERROR_ReportMissingLibrary(phyModelName, "Satellite");
#endif // SATELLITE_LIB
        }
        else if (strcmp(phyModelName, "PHY802.16") == 0) {
#ifdef ADVANCED_WIRELESS_LIB
            PHY_CreateAPhyForMac(node, nodeInput,
                                 interfaceIndex,
                                 &address, PHY802_16,
                                 &node->macData[interfaceIndex]->phyNumber);
#else // ADVANCED_WIRELESS_LIB
            ERROR_ReportMissingLibrary(phyModelName, "Advanced Wireless");
#endif // ADVANCED_WIRELESS_LIB
        }
        else if (strcmp(phyModelName, "PHY-LTE") == 0) {
#ifdef LTE_LIB
            LteLayer2PreInit(node, interfaceIndex, nodeInput);
            PHY_CreateAPhyForMac(node, nodeInput,
                                 interfaceIndex,
                                 &address, PHY_LTE,
                                 &node->macData[interfaceIndex]->phyNumber);
#else // LTE_LIB
            ERROR_ReportMissingLibrary(phyModelName, "PHY-LTE");
#endif // LTE_LIB
        }
        else if (strcmp(phyModelName, "PHY-JAMMING") == 0)
        {
#ifdef CYBER_LIB
            printf("AddNodeToIpv6Network(): PHY-JAMMING\n");
            PHY_CreateAPhyForMac(
                    node,
                    nodeInput,
                    interfaceIndex,
                    &address,
                    PHY_JAMMING,
                    &node->macData[interfaceIndex]->phyNumber);
#else // CYBER_LIB
            ERROR_ReportMissingLibrary(phyModelName, "Network Security");
#endif // CYBER_LIB
        }
        else
        {
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "Unknown PHY/Radio Model %s configured for "
                    "interface %d of Node %d.\n",
                    phyModelName,
                    interfaceIndex,
                    node->nodeId);
            ERROR_ReportError(errorStr);
        }

        //
        // bandwidth is set to the base data rate
        // (in case it's variable)
        //
        node->macData[interfaceIndex]->bandwidth =
            (PHY_GetTxDataRate(
                node,
                node->macData[interfaceIndex]->phyNumber) / 8);

        NetworkIpCreateQueues(node, nodeInput,
                              interfaceIndex);

#ifdef WIRELESS_LIB
        if (strcmp(macProtocolName, "CSMA") == 0) {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CSMA;
            MacCsmaInit(node, interfaceIndex, nodeInput);
        }
        else if (strcmp(macProtocolName, "MACA") == 0) {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_MACA;
            MacMacaInit(node, interfaceIndex, nodeInput);
        }
        else if (strcmp(macProtocolName, "TDMA") == 0)
        {
           node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_TDMA;
           MacTdmaInit(node, interfaceIndex,nodeInput,
                       subnetListIndex, nodesInSubnet);
        }
        else if (strcmp(macProtocolName, "ALOHA") == 0)
        {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_ALOHA;
            AlohaInit(node, interfaceIndex, nodeInput, nodesInSubnet);
        }
        else if (strcmp(macProtocolName, "GENERICMAC") == 0)
        {
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_GENERICMAC;
            GenericMacInit(node, interfaceIndex,nodeInput, nodesInSubnet);
        }
        else if (strcmp(macProtocolName, "MACDOT11") == 0)
        {
            node->macData[interfaceIndex]->macProtocol =
                MAC_PROTOCOL_DOT11;
            MacDot11Init(
                node,
                interfaceIndex,
                nodeInput,
                phyModel,
                subnetList,
                nodesInSubnet,
                subnetListIndex,
                0,
                0,
                FALSE,
                NETWORK_IPV6,
                subnetAddr,
                subnetPrefixLen);
        }
        else if (strcmp(macProtocolName, "MACDOT11e") == 0)
        {
            node->macData[interfaceIndex]->macProtocol =
                MAC_PROTOCOL_DOT11;
            MacDot11Init(
                node,
                interfaceIndex,
                nodeInput,
                phyModel,
                subnetList,
                nodesInSubnet,
                subnetListIndex,
                0,
                0,
                TRUE,
                NETWORK_IPV6,
                subnetAddr,
                subnetPrefixLen);
        }
#else // WIRELESS_LIB
        if ((strcmp(macProtocolName, "MAC802.11") == 0) ||
            (strcmp(macProtocolName, "CSMA") == 0) ||
            (strcmp(macProtocolName, "MACA") == 0) ||
            (strcmp(macProtocolName, "MACDOT11") == 0) ||
            (strcmp(macProtocolName, "MACDOT11e") == 0) ||
            (strcmp(macProtocolName, "TDMA") == 0) ||
            (strcmp(macProtocolName, "ALOHA") == 0) ||
            (strcmp(macProtocolName, "GENERICMAC") == 0))
        {
            ERROR_ReportMissingLibrary(macProtocolName, "Wireless");
        }
#endif // WIRELESS_LIB

        else if (strcmp(macProtocolName, "MAC802.16") == 0) {
#ifdef ADVANCED_WIRELESS_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_DOT16;
            MacDot16Init(node, interfaceIndex, nodeInput,
                         macProtocolName, nodesInSubnet, subnetListIndex);
#else // ADVANCED_WIRELESS_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Advanced Wireless");
#endif // ADVANCED_WIRELESS_LIB
        }

        else if (strcmp(macProtocolName, "MAC-LTE") == 0) {
#ifdef LTE_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_LTE;
            LteLayer2Init(node, interfaceIndex, nodeInput);
#else // LTE_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "MAC-LTE");
#endif // LTE_LIB
        }
        else if (strcmp(macProtocolName, "SATELLITE-BENTPIPE") == 0) {
#ifdef SATELLITE_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_SATELLITE_BENTPIPE;

            NodeAddress linkAddress =
                MAC_VariableHWAddressToFourByteMacAddress(node,
                                                          &node->macData[interfaceIndex]->macHWAddr);

            MacSatelliteBentpipeInit(node,
                                     interfaceIndex,
                                     linkAddress,
                                     nodeInput,
                                     nodesInSubnet);
#else //SATELLITE_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Satellite");
#endif // SATELLITE_LIB
        }
        else if (strcmp(macProtocolName, "GSM") == 0)
        {
#ifdef CELLULAR_LIB
            // Set GSM Parameters By their Default Value
            node->gsmNodeParameters = (GSMNodeParameters *)
            MEM_malloc(sizeof(GSMNodeParameters));
            node->gsmNodeParameters->lac = GSM_INVALID_LAC_IDENTITY;
            node->gsmNodeParameters->cellIdentity = GSM_INVALID_CELL_IDENTITY;

            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_GSM;
            MacGsmInit(node, interfaceIndex, nodeInput, nodesInSubnet);
#else // CELLULAR_LIB
            ERROR_ReportMissingLibrary(macProtocolName, "Cellular");
#endif // CELLULAR_LIB
        }

        else if (strcmp(macProtocolName, "CELLULAR-MAC") == 0)
        {
#ifdef WIRELESS_LIB
            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_CELLULAR;
            MacCellularInit(node,interfaceIndex, nodeInput);
#else
            ERROR_ReportMissingLibrary(macProtocolName, "Wireless");
#endif //Wireless Lib
        }
        else if (strcmp(macProtocolName, "MAC-WORMHOLE") == 0)
        {
#ifdef CYBER_LIB
            ERROR_ReportError("Wormhole attack simulation is not "
                              "supported for IPv6 network");
            /*
            if (nodesInSubnet < 2)
            {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr, "There are %d nodes (fewer than two) in "
                        "WORMHOLE subnet "
                        "%d", nodesInSubnet, subnetAddress);

                ERROR_ReportError(errorStr);
            }

            node->macData[interfaceIndex]->macProtocol = MAC_PROTOCOL_NETIA_WORMHOLE;
            MacWormholeInit(node,
                            nodeInput,
                            interfaceIndex,
                            interfaceAddress,
                            subnetAddress,
                            numHostBits,
                            subnetList,
                            nodesInSubnet,
                            subnetListIndex);
            */
#else // CYBER_LIB
            ERROR_ReportMissingLibrary(macProtocolName,
                                       "Network Security");
#endif // CYBER_LIB
        }
        else {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage, "Unknown MAC protocol %s\n",
                    macProtocolName);
            ERROR_ReportError(errorMessage);

            //MAC_InitUserMacProtocol(
            //    node, nodeInput,
            //    macProtocolName,
            //    interfaceIndex);
        }//if//
        node->macData[interfaceIndex]->mplsVar = NULL;
    }//if//
}

// -------------------------------------------------------------------------
// FUNCTION: MakeSureInterfaceAddressWithinRange
// PURPOSE:  Make sure the interface address is within allowable range.
// -------------------------------------------------------------------------
static void //inline
MakeSureInterfaceAddressWithinRange(NodeAddress interfaceAddress,
                                    NodeAddress subnetAddress,
                                    int numHostBits)
{
    NodeAddress minAddress;
    NodeAddress maxAddress;

    minAddress = subnetAddress + 1;
    maxAddress = subnetAddress + (NodeAddress) (pow(2.0, numHostBits) - 2);

    if (interfaceAddress > maxAddress || interfaceAddress < minAddress)
    {
        char buf[MAX_STRING_LENGTH];
        char addr1[MAX_STRING_LENGTH];
        char addr2[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(interfaceAddress, addr1);
        IO_ConvertIpAddressToString(subnetAddress, addr2);

        sprintf(buf, "interface address %s is out of range for subnet %s\n",
                     addr1, addr2);
        ERROR_ReportError(buf);
    }
}


// -------------------------------------------------------------------------
// FUNCTION: MakeSureSubnetAddressMatch
// PURPOSE:  Make sure that two subnet addresses match.
// -------------------------------------------------------------------------
static void //inline//
MakeSureSubnetAddressMatch(NodeAddress interfaceAddress1,
                           NodeAddress interfaceAddress2,
                           NodeAddress subnetAddress1,
                           NodeAddress subnetAddress2)
{
    if (subnetAddress1 != subnetAddress2)
    {
        char buf[MAX_STRING_LENGTH];
        char addr1[MAX_STRING_LENGTH];
        char addr2[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(interfaceAddress1, addr1);
        IO_ConvertIpAddressToString(interfaceAddress2, addr2);

        sprintf(buf, "Subnet address for %s and %s does not match.\n",
                     addr1, addr2);
        ERROR_ReportError(buf);
    }
}

// -------------------------------------------------------------------------
// FUNCTION: MakeSureSubnetMaskMatch
// PURPOSE:  Make sure that two subnet masks match.
// -------------------------------------------------------------------------
static void //inline//
MakeSureSubnetMaskMatch(NodeAddress interfaceAddress1,
                        NodeAddress interfaceAddress2,
                        NodeAddress subnetMask1,
                        NodeAddress subnetMask2)
{
    if (subnetMask1 != subnetMask2)
    {
        char buf[MAX_STRING_LENGTH];
        char addr1[MAX_STRING_LENGTH];
        char addr2[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(interfaceAddress1, addr1);
        IO_ConvertIpAddressToString(interfaceAddress2, addr2);

        sprintf(buf, "Subnet mask for %s and %s does not match.\n",
                     addr1, addr2);
        ERROR_ReportError(buf);
    }
}


// -------------------------------------------------------------------------
// FUNCTION: MakeSureNumHostBitsMatch
// PURPOSE:  Make sure that two number of host bits value match.
// -------------------------------------------------------------------------
static void //inline//
MakeSureNumHostBitsMatch(NodeAddress interfaceAddress1,
                         NodeAddress interfaceAddress2,
                         int numHostBits1,
                         int numHostBits2)
{
    if (numHostBits1 != numHostBits2)
    {
        char buf[MAX_STRING_LENGTH];
        char addr1[MAX_STRING_LENGTH];
        char addr2[MAX_STRING_LENGTH];

        IO_ConvertIpAddressToString(interfaceAddress1, addr1);
        IO_ConvertIpAddressToString(interfaceAddress2, addr2);

        sprintf(buf, "Number of host bits for %s and %s does not match.\n",
                     addr1, addr2);
        ERROR_ReportError(buf);
    }
}


// -------------------------------------------------------------------------
// FUNCTION: MakeSureMacProtocolMatch
// PURPOSE:  Make sure that two MAC protocols match.
// -------------------------------------------------------------------------
static void //inline//
MakeSureMacProtocolMatch(NodeId node1,
                         NodeId node2,
                         int intface1,
                         int intface2,
                         char *macProtocolName1,
                         char *macProtocolName2)
{
    if (strcmp(macProtocolName1, macProtocolName2) != 0)
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "Interface %d of Node %d and Interface %d of Node %d"
                     " lie in same subnet but have diffrent MAC protocol.\n",
                     intface1, node1,
                     intface2, node2);
        ERROR_ReportError(buf);
    }
}

#ifdef ADDON_BOEINGFCS
// --------------------------------------------------------------------------
// FUNCTION: MacInterfaceStatsInitialize
// PURPOSE:  Initialization the MAC layer interface based stats.
// --------------------------------------------------------------------------
void MacInterfaceStatsInitialize(Node* node)
{
    while (node != NULL)
    {
        if (node->partitionId != node->partitionData->partitionId)
        {
            node = node->nextNodeData;
            continue;
        }

        for (int interfaceIndex = 0;
            interfaceIndex < node->numberInterfaces;
            interfaceIndex++)
        {
            node->macData[interfaceIndex]->ifInOctets = 0;
            node->macData[interfaceIndex]->ifOutOctets = 0;
            node->macData[interfaceIndex]->ifInErrors = 0;
            node->macData[interfaceIndex]->ifOutErrors = 0;
            node->macData[interfaceIndex]->ifHCInOctets = 0;
            node->macData[interfaceIndex]->ifHCOutOctets = 0;
            if (node->macData[interfaceIndex]->faultCount > 0)
            {
                node->macData[interfaceIndex]->ifOperStatus = 2;
            }
            else
            {
                node->macData[interfaceIndex]->ifOperStatus = 1;
            }
        }
        node = node->nextNodeData;
    }
}
#endif

// --------------------------------------------------------------------------
// FUNCTION: MAC_AddHierarchyInfo
// PURPOSE:  When enabled, add several objects to the dynamic hierarchy
//           for mac properties (mac_type)
//           also see MacInterfaceStatsInitialize ()
// --------------------------------------------------------------------------
void MAC_AddHierarchyProperties (Node* firstNode)
{
    D_Hierarchy* h = &firstNode->partitionData->dynamicHierarchy;
    std::string path;
    BOOL createPath = FALSE;
    if (h->IsEnabled())
    {
        Node * node;
        NodePointerCollectionIter nodeIter;
        // Iterate through nodes
        for (nodeIter = firstNode->partitionData->allNodes->begin ();
            nodeIter != firstNode->partitionData->allNodes->end ();
            nodeIter++)
        {
            node = *nodeIter;
            // Only create properties in hierarchy if node is local
            if (node->partitionId != node->partitionData->partitionId)
                continue;

            for (int interfaceIndex = 0;
                interfaceIndex < node->numberInterfaces;
                interfaceIndex++)
            {
                // create path for interface's mac.
                createPath =
                    h->CreateNodeInterfacePath(
                    node,
                    interfaceIndex,
                    "macType",
                    path);
                if (createPath)
                {
                    // Add to the hierarchy
                    node->macData[interfaceIndex]->macProtocolDynamic =
                        node->macData[interfaceIndex]->macProtocol;
                    h->AddObject(
                        path,
                        new D_UInt32Obj(&node->macData[interfaceIndex]->macProtocolDynamic));

                    h->SetWriteable(path, FALSE);
                    h->SetExecutable(path, FALSE);
                }
            }
        }
    }
}

// --------------------------------------------------------------------------
// FUNCTION: ProcessLinkLine
// PURPOSE:  Parse the Link line from the configuration file.
// PARAMETERS:
// + firstNode      : Node *firstNode: first node's pointer in the partition
// + nodeInput      : const NodeInput* nodeInput: Pointer to node input.
// + nodeList       : const char[] nodeList: node list in LINK configuration.
// + siteCounter    : unsigned short : site Counter
// RETURN : None
// --------------------------------------------------------------------------
static void //inline//
ProcessLinkLine(
    Node *firstNode,
    const NodeInput *nodeInput,
    const char nodeList[],
    unsigned short& siteCounter)
{
    char errorString[MAX_STRING_LENGTH];
    NodeAddress nodeId1, nodeId2;
    Node* node1;
    Node* node2;
    Node* referenceNode1;
    Node* referenceNode2;
    int retVal = 0;

    NodeAddress interfaceAddress1 = 0;
    NodeAddress interfaceAddress2 = 0;

    NodeAddress subnetAddress1 = 0;
    NodeAddress subnetAddress2 = 0;

    NodeAddress subnetMask1 = 0;
    NodeAddress subnetMask2 = 0;

    int numHostBits1 = 0;
    int numHostBits2 = 0;

    BOOL IsIPv4LinkRequired = TRUE;

    int interfaceIndex1;
    int interfaceIndex2;

    BOOL isNewInterface1 = TRUE;
    BOOL isNewInterface2 = TRUE;

    NetworkProtocolType netProtoType1;
    NetworkProtocolType netProtoType2;

    in6_addr globalAddr1;
    memset(&globalAddr1, 0, sizeof(in6_addr));
    in6_addr globalAddr2;
    memset(&globalAddr2, 0, sizeof(in6_addr));
    in6_addr subnetAddr1;
    memset(&subnetAddr1, 0, sizeof(in6_addr));
    in6_addr subnetAddr2;
    memset(&subnetAddr2, 0, sizeof(in6_addr));
    unsigned int subnetPrefixLen1 = 0;
    unsigned int subnetPrefixLen2 = 0;

    Mac802Address linkAddr1;
    Mac802Address linkAddr2;
    MacHWAddress tempHWAddr;
    Address addr;

    BOOL IsIPv6LinkRequired = TRUE;

    int interfaceIndex;
    Address address;

    AddressMapType *map = firstNode->partitionData->addressMapPtr;

    PartitionData* partitionData = firstNode->partitionData;

    // Okay to use inefficient sscanf() here because
    // ProcessLinkLine() is called only the number of LINK
    // lines times.

    retVal = sscanf(nodeList,
                    "{ %u, %u }",
                    &nodeId1,
                    &nodeId2);

    if (retVal != 2)
    {
        sprintf(errorString, "Bad LINK declaration:\n");
        ERROR_ReportError(errorString);
    }

    node1 = MAPPING_GetNodePtrFromHash(partitionData->nodeIdHash, nodeId1);
    node2 = MAPPING_GetNodePtrFromHash(partitionData->nodeIdHash, nodeId2);

    // The following code is for parallel QualNet, where either or both node
    // pointers might be NULL.

    referenceNode1 = node1;
    referenceNode2 = node2;

#ifdef PARALLEL //Parallel
    if (node1 == NULL) {
        referenceNode1 = MAPPING_GetNodePtrFromHash(
            partitionData->remoteNodeIdHash,
            nodeId1);
        assert(referenceNode1 != NULL);
    }
    if (node2 == NULL) {
        referenceNode2 = MAPPING_GetNodePtrFromHash(
            partitionData->remoteNodeIdHash,
            nodeId2);
        assert(referenceNode2 != NULL);
    }

#else  //elseParallel
    ERROR_Assert((node1 != NULL && node2 != NULL),
                 "One node in the link doesn't exist");
#endif //endParallel

    interfaceIndex1 = referenceNode1->numberInterfaces;

    netProtoType1 = MAPPING_GetNetworkProtocolTypeForInterface(
                                            map, nodeId1, interfaceIndex1);

    if (netProtoType1 == IPV4_ONLY || netProtoType1 == DUAL_IP
        || netProtoType1 == GSM_LAYER3 || netProtoType1 == CELLULAR)
    {
        if (DEBUG)
        {
            printf("getting interfaceInfo for node %d, interface %d\n",
                   referenceNode1->nodeId, interfaceIndex1);
        }

        MAPPING_GetInterfaceInfoForInterface(referenceNode1,
                                             referenceNode1->nodeId,
                                             interfaceIndex1,
                                             &interfaceAddress1,
                                             &subnetAddress1,
                                             &subnetMask1,
                                             &numHostBits1);

        if (interfaceAddress1 == ANY_ADDRESS)
        {
            char buff[MAX_STRING_LENGTH];
            sprintf(buff,
              "IPv4 address is not configured for interface %d of Node %d\n",
              interfaceIndex1,
              nodeId1);
            ERROR_ReportError(buff);
        }

        MakeSureInterfaceAddressWithinRange(interfaceAddress1,
                                            subnetAddress1,
                                            numHostBits1);

        if (node1 != NULL)
        {
            if (DEBUG)
            {
                char interfaceAddr[MAX_STRING_LENGTH];
                char subnetAddr[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(interfaceAddress1, interfaceAddr);
                IO_ConvertIpAddressToString(subnetAddress1, subnetAddr);

                printf("Node %d assigning address (%s, %s) to interface %d\n",
                       node1->nodeId,
                       interfaceAddr,
                       subnetAddr,
                       interfaceIndex1);
            }

            MacAddNewInterface(
                node1,
                interfaceAddress1,
                numHostBits1,
                &interfaceIndex,
                nodeInput,
                "Link",
                isNewInterface1);

            isNewInterface1 = FALSE;

            SetIPv4AddressInfo(&address, interfaceAddress1);
        }
#ifdef PARALLEL //Parallel
        else
        {
            referenceNode1->numberInterfaces = interfaceIndex1 + 1;
        }
#endif //endParallel
    }
    else
    {
        IsIPv4LinkRequired = FALSE;
    }

    interfaceIndex2 = referenceNode2->numberInterfaces;

    netProtoType2 = MAPPING_GetNetworkProtocolTypeForInterface(
                                            map, nodeId2, interfaceIndex2);

    if (netProtoType2 == IPV4_ONLY || netProtoType2 == DUAL_IP
        || netProtoType2 == GSM_LAYER3 || netProtoType2 == CELLULAR)
    {
        if (DEBUG)
        {
            printf("getting interfaceInfo for node %d, interface %d\n",
                   referenceNode2->nodeId, interfaceIndex2);
        }

        MAPPING_GetInterfaceInfoForInterface(referenceNode2,
                                             referenceNode2->nodeId,
                                             interfaceIndex2,
                                             &interfaceAddress2,
                                             &subnetAddress2,
                                             &subnetMask2,
                                             &numHostBits2);

        if (interfaceAddress2 == ANY_ADDRESS)
        {
            char buff[MAX_STRING_LENGTH];
            sprintf(buff,
              "IPv4 address is not configured for interface %d of Node %d\n",
              interfaceIndex2,
              nodeId2);
            ERROR_ReportError(buff);
        }


        MakeSureInterfaceAddressWithinRange(interfaceAddress2,
                                            subnetAddress2,
                                            numHostBits2);


        if (node2 != NULL)
        {
            if (DEBUG)
            {
                char interfaceAddr[MAX_STRING_LENGTH];
                char subnetAddr[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(interfaceAddress2, interfaceAddr);
                IO_ConvertIpAddressToString(subnetAddress2, subnetAddr);

                printf("Node %d assigning address (%s, %s) to interface %d\n",
                       node2->nodeId,
                       interfaceAddr,
                       subnetAddr,
                       interfaceIndex2);
            }

            MacAddNewInterface(
                node2,
                interfaceAddress2,
                numHostBits2,
                &interfaceIndex,
                nodeInput,
                "Link",
                isNewInterface2);

            isNewInterface2 = FALSE;

            SetIPv4AddressInfo(&address, interfaceAddress2);

    #ifdef ENTERPRISE_LIB
            MacAddVlanInfoForThisInterface(
                node2, interfaceIndex, &address, nodeInput);
    #endif // ENTERPRISE_LIB
        }
#ifdef PARALLEL //Parallel
        else
        {
            referenceNode2->numberInterfaces = interfaceIndex2 + 1;
        }
#endif //endParallel
    }
    else
    {
        IsIPv4LinkRequired = FALSE;
    }

    if (netProtoType1 == IPV6_ONLY || netProtoType1 == DUAL_IP)
    {
        siteCounter++;

        MAPPING_GetIpv6InterfaceInfoForInterface(referenceNode1,
                                                 referenceNode1->nodeId,
                                                 interfaceIndex1,
                                                 &globalAddr1,
                                                 &subnetAddr1,
                                                 &subnetPrefixLen1);

        // several address related validity should be added
        if (node1 != NULL)
        {
            if (DEBUG)
            {
                char addr1[MAX_STRING_LENGTH] = {0, 0};
                IO_ConvertIpAddressToString(&globalAddr1, addr1);
                printf("Node %d assigning address (%s) to interface %d\n",
                       node1->nodeId,
                       addr1,
                       node1->numberInterfaces);
            }

            MacAddNewInterface(
                node1,
                &globalAddr1,
                &subnetAddr1,
                subnetPrefixLen1,
                &interfaceIndex1,
                nodeInput,
                siteCounter,
                "Link",
                isNewInterface1);

            SetIPv6AddressInfo(&address, globalAddr1);

    #ifdef ENTERPRISE_LIB
            MacAddVlanInfoForThisInterface(
                node1, interfaceIndex1, &address, nodeInput);
    #endif // ENTERPRISE_LIB
        }
    #ifdef PARALLEL //Parallel
        else
        {
            referenceNode1->numberInterfaces = interfaceIndex1 + 1;
        }
    #endif //endParallel
    }
    else
    {
        IsIPv6LinkRequired = FALSE;
    }

    if (netProtoType2 == IPV6_ONLY || netProtoType2 == DUAL_IP)
    {
        MAPPING_GetIpv6InterfaceInfoForInterface(referenceNode2,
                                                 referenceNode2->nodeId,
                                                 interfaceIndex2,
                                                 &globalAddr2,
                                                 &subnetAddr2,
                                                 &subnetPrefixLen2);
        if (node2 != NULL)
        {
            if (DEBUG)
            {
                char addr2[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(&globalAddr2, addr2);
                printf("Node %d assigning address (%s) to interface %d\n",
                       node2->nodeId,
                       addr2,
                       node2->numberInterfaces);
            }

            MacAddNewInterface(
                node2,
                &globalAddr2,
                &subnetAddr2,
                subnetPrefixLen2,
                &interfaceIndex2,
                nodeInput,
                siteCounter,
                "Link",
                isNewInterface2);

            SetIPv6AddressInfo(&address, globalAddr2);

    #ifdef ENTERPRISE_LIB
            MacAddVlanInfoForThisInterface(
                node2, interfaceIndex2, &address, nodeInput);
    #endif // ENTERPRISE_LIB
        }
#ifdef PARALLEL //Parallel
        else
        {
            referenceNode2->numberInterfaces = interfaceIndex2 + 1;
        }
#endif //endParallel
    }
    else
    {
        IsIPv6LinkRequired = FALSE;
    }

    if (IsIPv4LinkRequired)
    {
        if (node1 != NULL)
        {
            addr.networkType = NETWORK_IPV4;
            addr.interfaceAddr.ipv4 = interfaceAddress2;

            MacConfigureHWAddress(node1, nodeId2, interfaceIndex2, nodeInput,
                                  &tempHWAddr, "Link", NETWORK_IPV4, addr);

            ConvertVariableHWAddressTo802Address(
                                        node1, &tempHWAddr, &linkAddr2);

            CreateLink(node1, nodeInput, interfaceIndex1, interfaceIndex2,
                nodeId1, interfaceAddress1, nodeId2, interfaceAddress2,
                subnetAddress1, numHostBits1, linkAddr2);
        }
        if (node2 != NULL)
        {
            addr.networkType = NETWORK_IPV4;
            addr.interfaceAddr.ipv4 = interfaceAddress1;

            MacConfigureHWAddress(node2, nodeId1, interfaceIndex1, nodeInput,
                                  &tempHWAddr, "Link", NETWORK_IPV4, addr);

            ConvertVariableHWAddressTo802Address(
                                    node2, &tempHWAddr, &linkAddr1);

            CreateLink(node2, nodeInput, interfaceIndex2, interfaceIndex1,
                nodeId1, interfaceAddress1, nodeId2, interfaceAddress2,
                subnetAddress2, numHostBits2, linkAddr1);
        }
    }
    else if (IsIPv6LinkRequired)
    {
        if (node1 != NULL)
        {
            addr.networkType = NETWORK_IPV6;
            addr.interfaceAddr.ipv6 = globalAddr2;

            MacConfigureHWAddress(node1, nodeId2, interfaceIndex2, nodeInput,
                                  &tempHWAddr, "Link", NETWORK_IPV6, addr);

            ConvertVariableHWAddressTo802Address(node1,
                                 &node1->macData[interfaceIndex1]->macHWAddr,
                                 &linkAddr1);

            ConvertVariableHWAddressTo802Address(node1, &tempHWAddr,
                                                                &linkAddr2);

           CreateIpv6Link(node1, nodeInput, interfaceIndex1, interfaceIndex2,
                           nodeId1, &globalAddr1, nodeId2, &globalAddr2,
                           linkAddr1, linkAddr2,
                           subnetAddr1, subnetPrefixLen1);
        }
        if (node2 != NULL)
        {
            addr.networkType = NETWORK_IPV6;
            addr.interfaceAddr.ipv6 = globalAddr1;

            MacConfigureHWAddress(node2, nodeId1, interfaceIndex1, nodeInput,
                                  &tempHWAddr, "Link", NETWORK_IPV6, addr);

            ConvertVariableHWAddressTo802Address(node2, &tempHWAddr,
                                                                &linkAddr1);

             ConvertVariableHWAddressTo802Address(node2,
                                 &node2->macData[interfaceIndex2]->macHWAddr,
                                 &linkAddr2);

           CreateIpv6Link(node2, nodeInput, interfaceIndex2, interfaceIndex1,
                           nodeId1, &globalAddr1, nodeId2, &globalAddr2,
                           linkAddr1, linkAddr2,
                           subnetAddr2, subnetPrefixLen2);
        }
    }
    else
    {
        char buff[MAX_STRING_LENGTH];
        sprintf(buff,
               "NETWORK-PROTOCOL mismatch for LINK b/w Node %d and Node%d\n",
               nodeId1,
               nodeId2);
        ERROR_ReportError(buff);
    }
}

// --------------------------------------------------------------------------
// FUNCTION: FindGuiSubnetTypeForMacProtocol
// PURPOSE:  Fid the GUI Subnet Type from mac-protocol string.
// PARAMETERS:
// + macProtocolName    : char* : mac-protocol String
// RETURN               : GuiSubnetTypes : GUI Subnet Type
// --------------------------------------------------------------------------
GuiSubnetTypes FindGuiSubnetTypeForMacProtocol(char *macProtocolName)
{
    if (!strcmp(macProtocolName, "SWITCHED-ETHERNET"))
    {
        return GUI_WIRED_SUBNET;
    }
    else if (!strcmp(macProtocolName, "MAC802.3"))
    {
        return GUI_WIRED_SUBNET;
    }
    else if (!strcmp(macProtocolName, "SATCOM"))
    {
        return GUI_SATELLITE_NETWORK;
    }
#ifdef ADDON_BOEINGFCS
    else if (strcmp(macProtocolName, "MAC-CES-WNW-MDL") == 0)
    {
        return GUI_WIRELESS_SUBNET;
    }
    else if (strcmp(macProtocolName, "USAP") == 0)
    {
        return GUI_WIRELESS_SUBNET;
    }
    else if (!strcmp(macProtocolName, "MAC-CES-SRW"))
    {
      return GUI_WIRELESS_SUBNET;
    }
    else if (!strcmp(macProtocolName, "MAC-CES-WIN-T-NCW"))
    {
        return GUI_SATELLITE_NETWORK;
    }
    else if (strcmp(macProtocolName, "MAC-CES-WIN-T-GBS") == 0)
    {
        return GUI_SATELLITE_NETWORK;
    }
#endif // ADDON_BOEINGFCS
#ifdef SATELLITE_LIB
    else if (!strcmp(macProtocolName, "SATTSM"))
    {
        return GUI_SATELLITE_NETWORK;
    }
    else if (!strcmp(macProtocolName, "ANE"))
    {
        return GUI_SATELLITE_NETWORK;
    }
    else if (strcmp(macProtocolName, "SATELLITE-BENTPIPE") == 0)
    {
        return GUI_SATELLITE_NETWORK;
    }
#endif // SATELLITE_LIB
    else
    {
        return GUI_WIRELESS_SUBNET;
    }
}


// --------------------------------------------------------------------------
// FUNCTION: ProcessSubnetLine
// PURPOSE:  Parse the subnet line from the configuration file.
// PARAMETERS:
// + firstNode         : Node*  : Pointer to the First Node
// + nodeInput         : const  NodeInput* : Pointer to node input
// + ipv4SubnetAddress : const NodeAddress: ipv4 subnet address.
// + numHostBits       : int numHostBits : num of host bits in ipv4 subnet
// + ipv6SubnetAddress : const in6_addr*: ipv6 subnet address.
// + subnetPrefixLen   : unsigned int subnetPrefixLen : subnetPrefixLen
// + pNodeList         : const char* : Pointer to  Node list
// + subnetListData    : PartitionSubnetList* subnetListData
// + subnetIndex       : int& subnetIndex : subnet Index
// + siteCounter       : unsigned short&  : site Counter
// RETURN : None
// --------------------------------------------------------------------------
void ProcessSubnetLine(Node *firstNode,
                  const NodeInput *nodeInput,
                  const NodeAddress ipv4SubnetAddress,
                  int numHostBits,
                  const in6_addr* ipv6SubnetAddress,
                  unsigned int subnetPrefixLen,
                  const char *pNodeList,
                  PartitionSubnetList* subnetListData,
                  int& subnetIndex,
                  unsigned short& siteCounter)
{
    BOOL wasFound;
    int lineLen = NI_GetMaxLen(nodeInput);
    char errorString[MAX_STRING_LENGTH];
    char *nodeIdList;

    int nodeId;
    int maxNodeId;
    const char* p = pNodeList;
    int addCount = 0;
    int count;
    char* endOfList;
    Node* node;
    SubnetMemberData* subnetList;

    // For IO_GetDelimitedToken
    char delims[] = "{,} \n\t";
    char* next;
    char* token;
    char iotoken[MAX_STRING_LENGTH];

    char macProtocolName[MAX_STRING_LENGTH];

    unsigned int maxipv4SubnetListSize;
    unsigned int maxipv6SubnetListSize;

    AddressMapType *map = firstNode->partitionData->addressMapPtr;

    BOOL isIPv6NodePresent = FALSE;

    if (firstNode == NULL)
    {
        return; // This partition has no nodes.
                // This will be a problem for partition 0.
    }

    maxipv4SubnetListSize = MAPPING_GetNumNodesInSubnet(
                                firstNode, ipv4SubnetAddress);

    maxipv6SubnetListSize = MAPPING_GetNumNodesInIPv6Network(
                             firstNode, *ipv6SubnetAddress, subnetPrefixLen);

    nodeIdList = (char*) MEM_malloc(lineLen);
    memset(nodeIdList,0,lineLen);
    strcpy(nodeIdList, p);

    // mark the end of the list
    // (required because Animator places tokens at the end)
    endOfList = strchr(nodeIdList, '}');
    if (endOfList != NULL)
        endOfList[0] = '\0';

    // Read first nodeId into nodeId variable.
    // Adding 1 to nodeIdList skips '{'.
    // Obtain MAC-PROTOCOL for the first nodeId.
    // This MAC-PROTOCOL will be used for all nodes in the subnet.

    nodeId = strtoul(nodeIdList + 1, NULL, 10);

    // Allocate an array big enough to hold all the nodes in the list.
    // This could be wasteful if people specify lots of N16 subnets
    // with only a few nodes in them.
    subnetList = (SubnetMemberData*) MEM_malloc(
                        sizeof(SubnetMemberData) *
                            (maxipv4SubnetListSize + maxipv6SubnetListSize));

    memset(subnetList, 0, sizeof(SubnetMemberData) *
                            (maxipv4SubnetListSize + maxipv6SubnetListSize));

    // Scan first nodeId from nodeIdList.  Remember we've already parsed
    // the first nodeId into the nodeId variable.  This call is just to
    // initialize IO_GetDelimitedToken().

    token = IO_GetDelimitedToken(iotoken, nodeIdList, delims, &next);

    while (1)
    {
        int intfaceIndex = -1;

        // Retrieve node pointer for nodeId and store in subnetList array.
        node = MAPPING_GetNodePtrFromHash(
                    firstNode->partitionData->nodeIdHash,
                    nodeId);

        if (node != NULL)
        {
            intfaceIndex = node->numberInterfaces;
        }

#ifdef PARALLEL //Parallel
#ifdef USE_MPI
        if (node == NULL)
        {
            node = MAPPING_GetNodePtrFromHash(
                firstNode->partitionData->remoteNodeIdHash,
                nodeId);

            intfaceIndex = node->numberInterfaces;
        }
#else
        if (node == NULL)
        {
            //Update Remote Node Interface Count
            Node* remoteNode = MAPPING_GetNodePtrFromHash(
                                 firstNode->partitionData->remoteNodeIdHash,
                                 nodeId);

            intfaceIndex = remoteNode->numberInterfaces;
            remoteNode->numberInterfaces++;
        }
#endif
#endif //endParallel

        subnetList[addCount].node   = node;
        subnetList[addCount].nodeId = nodeId;
        subnetList[addCount].interfaceIndex = intfaceIndex;

        addCount++;

        // Retrieve next token.

        token = IO_GetDelimitedToken(iotoken, next, delims, &next);

        if (!token)
        {
            // No more tokens.
            break;
        }

        if (strncmp("thru", token, 4) == 0)
        {
            // token is "thru".  Add nodeIds in range "x thru y" to
            // subnetList array.

            // Read in maximum nodeId.

            token = IO_GetDelimitedToken(iotoken, next, delims, &next);
            maxNodeId = strtoul(token, NULL, 10);

            nodeId++;
            while (nodeId <= maxNodeId)
            {
                // Retrieve node pointer for nodeId and store in subnetList array.
                node = MAPPING_GetNodePtrFromHash(
                            firstNode->partitionData->nodeIdHash,
                            nodeId);

                if (node != NULL)
                {
                    intfaceIndex = node->numberInterfaces;
                }

#ifdef PARALLEL //Parallel
#ifdef USE_MPI
                if (node == NULL) {
                    node = MAPPING_GetNodePtrFromHash(
                        firstNode->partitionData->remoteNodeIdHash,
                        nodeId);

                    intfaceIndex = node->numberInterfaces;
                }
#else
                if (node == NULL)
                {
                    //Update Remote Node Interface Count
                    Node* remoteNode = MAPPING_GetNodePtrFromHash(
                                 firstNode->partitionData->remoteNodeIdHash,
                                 nodeId);

                    intfaceIndex = remoteNode->numberInterfaces;
                    remoteNode->numberInterfaces++;
                }
#endif
#endif //endParallel

                subnetList[addCount].node   = node;
                subnetList[addCount].nodeId = nodeId;
                subnetList[addCount].interfaceIndex = intfaceIndex;

                addCount++;
                nodeId++;
            }

            // Done adding range of nodeIds to subnetList array.
            // Read next nodeId.

            token = IO_GetDelimitedToken(iotoken, next, delims, &next);

            if (!token)
            {
                // No more tokens.
                break;
            }
        }
        // Current token is a nodeId.  Parse it into nodeId variable.
        nodeId = strtoul(token, NULL, 10);
    }

    subnetListData->memberList = subnetList;
    subnetListData->numMembers = addCount;

    GuiSubnetTypes subnetType = GUI_WIRELESS_SUBNET;
    for (count = 0; count < addCount; count++)
    {
        NodeAddress interfaceAddress = (unsigned int)ANY_ADDRESS;
        NodeAddress subnetAddress = 0;
        NodeAddress subnetMask = 0;
        int hostBits = 0;
        in6_addr globalAddr;
        in6_addr v6subnetAddr;
        unsigned int prefixLen = 0;
        int intfIndex = subnetList[count].interfaceIndex;

        IO_ReadString(
                firstNode,
                subnetList[count].nodeId,
                intfIndex,
                nodeInput,
                "MAC-PROTOCOL",
                &wasFound,
                macProtocolName);

        if (!wasFound)
        {
            sprintf(errorString,
                   "No MAC-PROTOCOL specified for interface %d of Node %d\n",
                    intfIndex,
                    subnetList[count].nodeId);
            ERROR_ReportError(errorString);
        }

#ifdef ADDON_BOEINGFCS
        BOOL sipNode = FALSE;
        IO_ReadBool(
            subnetList[count].nodeId,
            ANY_INTERFACE,
            nodeInput,
            "CES-SINCGARS-SIP-NODE",
            &wasFound,
            &(sipNode));

        if (wasFound && sipNode)
        {
            sprintf(macProtocolName, "MAC-CES-SIP");
        }
#endif

        NetworkProtocolType netProtoType =
                        MAPPING_GetNetworkProtocolTypeForInterface(
                                   map, subnetList[count].nodeId, intfIndex);

        if (netProtoType == IPV4_ONLY || netProtoType == DUAL_IP
            || netProtoType == GSM_LAYER3 || netProtoType == CELLULAR)
        {

            MAPPING_GetInterfaceInfoForInterface(
                firstNode,
                subnetList[count].nodeId,
                intfIndex,
                &interfaceAddress,
                &subnetAddress,
                &subnetMask,
                &hostBits);

            if (interfaceAddress == ANY_ADDRESS)
            {
                char buff[MAX_STRING_LENGTH];
                sprintf(buff,
                  "IPv4 address is not configured for"
                  "interface %d of Node %d\n",
                  intfIndex,
                  subnetList[count].nodeId);
                ERROR_ReportError(buff);
            }

#ifdef SOCKET_INTERFACE
            SocketInterface_AddNodeToSubnet(
                firstNode, subnetList[count].nodeId, subnetAddress);
#endif

            subnetList[count].address.interfaceAddr.ipv4 = interfaceAddress;
            subnetList[count].address.networkType = NETWORK_IPV4;
        }

        if (netProtoType == IPV6_ONLY || netProtoType == DUAL_IP)
        {
            if (!isIPv6NodePresent)
            {
                isIPv6NodePresent = TRUE;
                siteCounter++;
            }
             MAPPING_GetIpv6InterfaceInfoForInterface(
                        firstNode,
                        subnetList[count].nodeId,
                        intfIndex,
                        &globalAddr,
                        &v6subnetAddr,
                        &prefixLen);

            if (netProtoType == IPV6_ONLY)
            {
                subnetList[count].address.interfaceAddr.ipv6 = globalAddr;
                subnetList[count].address.networkType = NETWORK_IPV6;
            }
        }

        if (count == 0 && firstNode->guiOption)
        {
            subnetType = FindGuiSubnetTypeForMacProtocol(macProtocolName);
            if (firstNode->partitionData->partitionId == 0)
            {
                // All parititons call MAC_Initializee, but we only
                // want p0 to create the GUI subnets.
                if (ipv4SubnetAddress != ANY_ADDRESS)
                {
                    GUI_CreateSubnet(subnetType,
                         ipv4SubnetAddress,
                         numHostBits,
                         p,
                         firstNode->getNodeTime());
                }
                else
                {
                    GUI_CreateSubnet(subnetType,
                                     (char*)ipv6SubnetAddress,
                                     subnetPrefixLen,
                                     p,
                                     firstNode->getNodeTime());
                }
            }
        }

        if (subnetList[count].node != NULL)
        {
            SimContext::setCurrentNode(subnetList[count].node);

            subnetList[count].node->enterInterface(intfIndex);

           BOOL isNewInterface = TRUE;
           if (firstNode->guiOption)
           {
               subnetType = FindGuiSubnetTypeForMacProtocol(macProtocolName);
           }

           if (netProtoType == IPV4_ONLY || netProtoType == DUAL_IP
            || netProtoType == GSM_LAYER3 || netProtoType == CELLULAR)
           {
                AddNodeToSubnet(subnetList[count].node,
                                nodeInput,
                                subnetAddress,
                                subnetList[count].address.interfaceAddr.ipv4,
                                hostBits,
                                macProtocolName,
                                subnetList,
                                addCount,
                                count,
                                isNewInterface,
                                (short)subnetIndex);
           }

           if (netProtoType == IPV6_ONLY || netProtoType == DUAL_IP)
           {
                if (netProtoType == DUAL_IP)
                {
                    isNewInterface = FALSE;
                }
                subnetList[count].node->exitInterface();
                subnetList[count].node->enterInterface(count);

                AddNodeToIpv6Network(subnetList[count].node,
                                     nodeInput,
                                     &globalAddr,
                                     &v6subnetAddr,
                                     prefixLen,
                                     macProtocolName,
                                     (short)subnetIndex,
                                     subnetList,
                                     addCount,
                                     count,
                                     siteCounter,
                                     isNewInterface);
               subnetList[count].node->exitInterface();
               subnetList[count].node->enterInterface(intfIndex);
           }

            if ((firstNode->guiOption && subnetType == GUI_WIRELESS_SUBNET)
             && (subnetList[count].node->partitionId ==
                subnetList[count].node->partitionData->partitionId))
            {
                 GUI_InitWirelessInterface(
                     subnetList[count].node,
                     subnetList[count].node->numberInterfaces - 1);
            }
            subnetList[count].node->exitInterface();

            SimContext::unsetCurrentNode();
        }
    }

    subnetIndex++;

    MEM_free(nodeIdList);
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_Initialize
// PURPOSE:  Initialization function for the MAC layer.
// --------------------------------------------------------------------------
void
MAC_Initialize(Node *firstNode,
               const NodeInput *nodeInput)
{
    int i, j;
    NodeInput faultInput;
    NodeInput bgInput;
    BOOL wasFound;
    char buf[MAX_STRING_LENGTH];
    char delims[] = "{,} \n\t";
    char* subnetAddressString =
        (char*) MEM_malloc(MAX_ADDRESS_STRING_LENGTH);
    static unsigned short siteCounter;
    Node *tempNode = firstNode;
    int numLinks = 0;

    //initialize subnet data structure on partition
    int numSubnets, subnetIndex;
    PartitionData* partitionData = firstNode->partitionData;
    PartitionSubnetData* subnetData = &partitionData->subnetData;
    //find number of subnets
    numSubnets = 0;
    for (i = 0; i < nodeInput->numLines; i++)
    {
        if (strcmp(nodeInput->variableNames[i], "SUBNET") == 0)
        {
            numSubnets++;
        }
    }
    subnetData->numSubnets = numSubnets;
    
    if (numSubnets >= 1)
    {
        subnetData->subnetList = (PartitionSubnetList*)
                MEM_malloc(sizeof(PartitionSubnetList) * numSubnets);
        memset(subnetData->subnetList, 0, sizeof(PartitionSubnetList)
            * numSubnets);
    }

    subnetIndex = 0;
    for (i = 0; i < nodeInput->numLines; i++)
    {
        char* currentLine = nodeInput->values[i];
        char *nextSubnetString = currentLine;

        BOOL isLink = FALSE;
        if (strcmp(nodeInput->variableNames[i], "SUBNET") == 0)
        {
        }
        else if (strcmp(nodeInput->variableNames[i], "LINK") == 0)
        {
            isLink = TRUE;
            numLinks++;
        }
        else
        {
            continue;
        }
            NodeAddress  ipv4subnetAddress = ANY_ADDRESS;
            int          numHostBits = 0;
            in6_addr     IPv6subnetAddress;
            unsigned int IPv6subnetPrefixLen = 0;
            BOOL IsIPv4AddressPresent = FALSE;
            BOOL IsIPv6AddressPresent = FALSE;
            char *p = strchr(currentLine, '{');

            unsigned int tla = 0;
            unsigned int nla = 0;
            unsigned int sla = 0;

            NetworkType networkType = NETWORK_INVALID;
            CLR_ADDR6(IPv6subnetAddress);

            //Dual-IP Improvements
            while (1)
            {
                subnetAddressString =
                    IO_GetDelimitedToken(subnetAddressString,
                            nextSubnetString, delims, &nextSubnetString);

                if (subnetAddressString[0] != 'N'
                    && (IO_FindCaseInsensitiveStringPos(
                                subnetAddressString, "SLA") == -1))
                {
                    break;
                }

               networkType = MAPPING_GetNetworkType(subnetAddressString);

                if (networkType == NETWORK_IPV6)
                {
                    if (IO_FindCaseInsensitiveStringPos(
                                subnetAddressString, "SLA") != -1)
                    {
                        IO_ParseNetworkAddress(
                                subnetAddressString, &tla, &nla, &sla);

                        MAPPING_CreateIpv6SubnetAddr(
                            tla,
                            nla,
                            sla,
                            &IPv6subnetPrefixLen,
                            &IPv6subnetAddress);

                    }
                    else
                    {
                        IO_ParseNetworkAddress(
                            subnetAddressString,
                            &IPv6subnetAddress,
                            &IPv6subnetPrefixLen);
                    }
                    IsIPv6AddressPresent = TRUE;
                }
                else
                {
                    IO_ParseNetworkAddress(
                        subnetAddressString,
                        &ipv4subnetAddress,
                        &numHostBits);

                    IsIPv4AddressPresent = TRUE;
                }
            }//Dual-IP Improvements

            if (!IsIPv6AddressPresent)
            {
                if (IsIPv4AddressPresent)
                {
                    Mapping_AutoCreateIPv6SubnetAddress(
                        ipv4subnetAddress,
                        subnetAddressString);

                    IO_ParseNetworkAddress(
                            subnetAddressString,
                            &IPv6subnetAddress,
                            &IPv6subnetPrefixLen);
                }
                else
                {
                    ERROR_ReportError("Neither IPv4 nor IPv6 network "
                                "specified in SUBNET/LINK Configuration\n");
                }
            }

            if (isLink == FALSE)
            {
                 ProcessSubnetLine(firstNode,
                                   nodeInput,
                                   ipv4subnetAddress,
                                   numHostBits,
                                   &IPv6subnetAddress,
                                   IPv6subnetPrefixLen,
                                   p,
                                   &subnetData->subnetList[subnetIndex],
                                   subnetIndex,
                                   siteCounter);
            }
            else
            {
                ProcessLinkLine(firstNode,
                                nodeInput,
                                p,
                                siteCounter);
            }
    }

   //debug creation of subnet data table
    if (DEBUG)
    {
        for (i = 0; i < numSubnets; i++)
        {
            if (subnetData->subnetList[i].memberList)
            {
                PartitionSubnetList* subnetListData = &subnetData->subnetList[i];
                int membersOnPartition = 0;
                for (j = 0; j < subnetListData->numMembers; j++)
                {
                    if (subnetListData->memberList[j].node != NULL)
                    {
                        membersOnPartition++;
                        if (partitionData->partitionId == 0)
                        {
                            printf("Node %d w/ interfaceIndex %d\n", subnetListData->memberList[j].nodeId,
                                subnetListData->memberList[j].interfaceIndex);
                        }
                    }
                }
                printf(" Subnet %d | %d out of %d members on partition %d\n", i,
                    membersOnPartition, subnetListData->numMembers, partitionData->partitionId);
            }
        }
    }

    MEM_free(subnetAddressString);
    subnetAddressString = NULL;

    if (firstNode->partitionData->guiOption) {
        GUI_InitializeInterfaces(nodeInput);
    }

    // Initialize switches by iterating over all nodes.
    while (tempNode)
    {
        if (tempNode->partitionId != tempNode->partitionData->partitionId)
        {
            tempNode = tempNode->nextNodeData;
            continue;
        }

        IO_ReadString(
            tempNode->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH",
            &wasFound,
            buf);

        if (wasFound)
        {
            if (!strcmp(buf, "YES"))
            {
#ifdef ENTERPRISE_LIB
    // It is assumed that all interfaces are initialized;
    // this call must be after SUBNET/LINK processing.
            Switch_Init(tempNode, nodeInput);
#else //ENTERPRISE_LIB
                sprintf(buf, "Node %u will act as default node and "
                    "not as SWITCH which requires multimedia_enterprise \n",
                    tempNode->nodeId);
                ERROR_ReportWarning(buf);
#endif // ENTERPRISE_LIB
            }
            else if (strcmp(buf, "NO"))
            {
                ERROR_ReportError(
                    "Error in SWITCH specification. "
                    "Expecting YES or NO\n");
            }
        }
        tempNode = tempNode->nextNodeData;
    }

    IO_ReadCachedFile(ANY_NODEID, ANY_ADDRESS, nodeInput,
        "FAULT-CONFIG-FILE", &wasFound, &faultInput);

    if (wasFound)
    {
        for (i = 0; i < faultInput.numLines; i++)
        {
            char faultString[MAX_STRING_LENGTH] = {0, 0};
            char interfaceAddrStr[MAX_STRING_LENGTH] = {0, 0};
            char startTimeStr[MAX_STRING_LENGTH] = {0, 0};
            char endTimeStr[MAX_STRING_LENGTH] = {0, 0};
            char cardFaultString[MAX_STRING_LENGTH] = {0, 0};
            char macAddrString[MAX_STRING_LENGTH] = {0, 0};
            char macHWAddrLength[20] = {0, 0};

            clocktype startTime;
            clocktype endTime;
            int numInputs;
            unsigned int hwLength = MAC_ADDRESS_DEFAULT_LENGTH;

            Address interfaceAddress;
            NodeId nodeId;
            int interfaceIndex = -1;
            memset(&interfaceAddress, 0, sizeof(Address));

            const char *currentLine = faultInput.inputStrings[i];
            numInputs = sscanf(currentLine, "%s %s %s %s %s %s %s",faultString,
                interfaceAddrStr, startTimeStr, endTimeStr, cardFaultString,
                macAddrString,macHWAddrLength);

            if (numInputs == 7)
            {
                hwLength = (unsigned int) strtoul(macHWAddrLength, NULL, 10);
            }

            if (numInputs < 4)
            {
                char errStr[2 * MAX_STRING_LENGTH];

                sprintf(errStr, "The format in the file should be:\n"
                    "\tINTERFACE-FAULT <interface ip address> <start time>"
                    " <end time> [INTERFACE-CARD-FAULT <new mac-address>"
                    "<address length in byte]\n"
                    "\tNote: INTERFACE-CARD-FAULT and replacement is "
                    "supported only when ARP is enabled");

                ERROR_ReportError(errStr);
            }

            if (strcmp(faultString, "INTERFACE-FAULT") == 0)
            {
                Message* newMsg = NULL;
                Node *faultyNode = NULL;

                if (MAPPING_GetNetworkType(interfaceAddrStr)
                        == NETWORK_IPV4)
                {
                    interfaceAddress.networkType = NETWORK_IPV4;
                    BOOL isNodeId;

                    IO_ParseNodeIdOrHostAddress(interfaceAddrStr,
                        &interfaceAddress.interfaceAddr.ipv4, &isNodeId);

                    if (isNodeId)
                    {
                        ERROR_ReportError("The first entry after "
                            "INTERFACE-FAULT must be an IP address");
                    }

                }
                else if (MAPPING_GetNetworkType(interfaceAddrStr)
                    == NETWORK_ATM)
                {

                    ERROR_ReportError("INTERFACE-FAULT is not"
                        "supported for ATM");
                }
                else
                {
                    // For IPv6 Network
                    NodeId isNodeId;
                    interfaceAddress.networkType = NETWORK_IPV6;
                    IO_ParseNodeIdOrHostAddress(
                        interfaceAddrStr,
                        &interfaceAddress.interfaceAddr.ipv6,
                        &isNodeId);

                    if (isNodeId)
                    {
                        ERROR_ReportError("The first entry after "
                            "INTERFACE-FAULT must be an IP address");
                    }
                }

                    nodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                 firstNode,
                                 interfaceAddress);

                    if (nodeId == INVALID_MAPPING)
                    {
                        ERROR_ReportError("The interface address specified "
                            "is not bound to any node");
                    }

                faultyNode = MAPPING_GetNodePtrFromHash(
                    firstNode->partitionData->nodeIdHash,
                    nodeId);

                if (faultyNode)
                {
                    MacFaultInfo* macFaultInfo;

                    if (MAPPING_GetNetworkType(interfaceAddrStr)
                            == NETWORK_IPV4)
                    {
                        interfaceIndex =
                                 NetworkIpGetInterfaceIndexFromAddress(
                                        faultyNode,
                                        interfaceAddress.interfaceAddr.ipv4);

                        if (interfaceIndex == -1)
                        {
                            char errStr[MAX_STRING_LENGTH];

                            sprintf(errStr,
                                    "No interface exists with inteface "
                                    "address: %s at node %u",
                                    interfaceAddrStr,
                                    faultyNode->nodeId);

                            ERROR_ReportError(errStr);
                        }
                    }
                    else
                    {
                        // For IPv6 Network
                        interfaceIndex = Ipv6GetInterfaceIndexFromAddress(
                            faultyNode,
                            &interfaceAddress.interfaceAddr.ipv6);
                        if (interfaceIndex == -1)
                        {
                            char errStr[MAX_STRING_LENGTH];

                            sprintf(errStr,
                                    "No interface exists with inteface "
                                    "address: %s at node %u",
                                    interfaceAddrStr,
                                    faultyNode->nodeId);

                            ERROR_ReportError(errStr);
                        }

                    }

                    //thirdString is one of these three means,
                    //line is for Random Fault
                    if (strstr(currentLine, "REPS") != NULL ||
                        strstr(currentLine, "START") != NULL ||
                        strstr(currentLine, "MTBF") != NULL)
                    {
                        MAC_RandFaultInit(faultyNode,
                                           interfaceIndex,
                                           currentLine);
                        continue;
                    }

                    startTime = TIME_ConvertToClock(startTimeStr);
                    endTime = TIME_ConvertToClock(endTimeStr);

                    // offset time by simulation start time
                    startTime -= getSimStartTime(firstNode);
                    endTime   -= getSimStartTime(firstNode);

                    if ((endTime > 0) && (endTime < startTime))
                    {
                        ERROR_ReportError("End time must be 0 or greater "
                                          "than startTime");
                    }

                    newMsg = MESSAGE_Alloc(
                                 faultyNode,
                                 MAC_LAYER,
                                 0,
                                 MSG_MAC_StartFault);

                    MESSAGE_SetInstanceId(newMsg, (short) interfaceIndex);

                    //this information is required to handle static and
                    //random fault by one pair of event message
                    MESSAGE_InfoAlloc(faultyNode, newMsg,
                                                       sizeof(MacFaultInfo));

                    macFaultInfo = (MacFaultInfo*)
                                        MESSAGE_ReturnInfo(newMsg);
                    macFaultInfo->faultType = STATIC_FAULT;

                    MESSAGE_Send(faultyNode, newMsg, startTime);

                    if (endTime > 0)
                    {
                        newMsg = MESSAGE_Alloc(faultyNode, MAC_LAYER, 0,
                                               MSG_MAC_EndFault);

                        MESSAGE_SetInstanceId(newMsg,
                             (short) interfaceIndex);

                        //this information is required to handle static
                        //and random fault by one pair of event message
                        MESSAGE_InfoAlloc(
                            faultyNode,
                            newMsg, sizeof(MacFaultInfo));
                        macFaultInfo =
                            (MacFaultInfo*) MESSAGE_ReturnInfo(newMsg);
                        macFaultInfo->faultType = STATIC_FAULT;

                        // INTERFACE-CARD-FAULT and replacement is supported
                        // only when ARP is enabled.
                        // Current ARP implementaion support Address
                        // Resolution between IP(v4) addresses to Ethernet
                        // (802.3) MAC addresses.

                        if (!strcmp(cardFaultString, "INTERFACE-CARD-FAULT"))
                        {
                            macFaultInfo->macHWAddress.hwLength =
                                (unsigned short)hwLength;

                            MacValidateAndSetHWAddress( macAddrString,
                                               &macFaultInfo->macHWAddress);

                            macFaultInfo->interfaceCardFailed = TRUE;
                            faultyNode->macData[interfaceIndex]
                                    ->interfaceCardFailed = TRUE;
                        }
                        MESSAGE_Send(faultyNode, newMsg, endTime);
                    }
                }
                else
                {
#ifndef PARALLEL //Parallel
                    ERROR_ReportError("Invalid IP address");
#endif //endParallel
                }
            }
            else
            {
                ERROR_ReportError("The only possible entry in the fault "
                                "configuration file is now INTERFACE-FAULT");
            }
        }
    }

    IO_ReadCachedFile(ANY_NODEID, ANY_ADDRESS, nodeInput,
        "BACKGROUND-TRAFFIC-CONFIG-FILE", &wasFound, &bgInput);

    if (wasFound)
    {
        Node* nextNode  = firstNode;

        for (i = 0; i < bgInput.numLines; i++)
        {
            const char *currentLine = bgInput.inputStrings[i];
            BgTraffic_Init(firstNode, currentLine);
        }

        // Fired the selfTimer message of BG traffic
        // for each node at each interface.
        while (nextNode != NULL)
        {
            int interfaceIndex;

            for (interfaceIndex = 0;
                interfaceIndex < nextNode->numberInterfaces;
                interfaceIndex++)
            {
                // Check that the node is on this partition
                if (nextNode->partitionId != firstNode->partitionData->partitionId)
                {
                    continue;
                }

                if (nextNode->macData[interfaceIndex]->bgMainStruct)
                {
                    BgTraffic_InvokeSelfMsgForFirstTimer(
                                            nextNode, interfaceIndex);

                    if (LINK_BG_TRAFFIC_DEBUG)
                    {
                        BgMainStruct* bgMainStruct = (BgMainStruct*)
                            nextNode->macData[interfaceIndex]->bgMainStruct;
                        BgTraffic_PrintSelfTimerMsgList(
                                        nextNode,
                                        interfaceIndex,
                                        bgMainStruct->bgSelfTimerMsgList);
                    }
                }
            }
            nextNode = nextNode->nextNodeData;
        }
    }


    MAC_AddHierarchyProperties (firstNode);

#ifdef ADDON_BOEINGFCS
    // FOR MIBS Initialization for the MAC stats.
    MacInterfaceStatsInitialize(firstNode);
#endif

    char value[MAX_STRING_LENGTH];

    sprintf(value, "%d", subnetData->numSubnets);
    SetOculaProperty(NULL, "/scenario/subnetCount", value);

    sprintf(value, "%d", numLinks);
    SetOculaProperty(NULL, "/scenario/linkCount", value);
}


// --------------------------------------------------------------------------
// FUNCTION: MAC_InformInterfaceStatusHandler
// PURPOSE:  Call the interface handler functions when interface
//           goes up and down.
// --------------------------------------------------------------------------
void MAC_InformInterfaceStatusHandler(Node *node,
                                      int interfaceIndex,
                                      MacInterfaceState state)
{
    ListItem *item;
    MacReportInterfaceStatus statusHandler;

    item = node->macData[interfaceIndex]->interfaceStatusHandlerList->first;

    while (item)
    {
        statusHandler = (MacReportInterfaceStatus) item->data;

        assert(statusHandler);

        (statusHandler)(node, interfaceIndex, state);

        item = item->next;
    }

    TunnelInformInterfaceStatusHandler(node,
                                 interfaceIndex,
                                 MAC_INTERFACE_DISABLE);
}



// --------------------------------------------------------------------------
// FUNCTION: MAC_ProcessDisableInterface
// PURPOSE:  Disable the specific interface
// --------------------------------------------------------------------------
void MAC_ProcessDisableInterface(Node* node,
                                 Message* msg,
                                 int   interfaceIndex)
{
    int i;
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    NetworkForwardingTable *forwardTable = &(ip->forwardTable);

    MacFaultInfo* faultInfo = (MacFaultInfo*) MESSAGE_ReturnInfo(msg);
    assert(faultInfo != NULL);

    // this condition is required to increment the fault count.
    // it increments only in case of static and random fault.
    if (faultInfo->faultType == STATIC_FAULT
        || faultInfo->faultType == RANDOM_FAULT)
    {
        node->macData[interfaceIndex]->faultCount++;
    }
    if (!MAC_InterfaceIsEnabled(node, interfaceIndex))
    {
        return;
    }
#ifdef ADDON_BOEINGFCS
    // update the stats.
    node->macData[interfaceIndex]->ifOperStatus = 2;
#endif
    if (DEBUG)
    {
        char time[20];
        TIME_PrintClockInSecond(node->getNodeTime(), time, node);
        printf("Making interface disable: node %u interface %d\n",
            node->nodeId, interfaceIndex);
        printf("\tCurrent time is %s\n", time);
    }

    if (ArpIsEnable(node, interfaceIndex))
    {
        ArpHandleHWInterfaceFault(node, interfaceIndex, TRUE);
    }

#ifdef ADDON_DB
    // if this interface is already enabled, then it should not
    // trigger a DB insert
    BOOL alreadyDisabled = !NetworkIpInterfaceIsEnabled(node, interfaceIndex);
#endif

    MAC_DisableInterface(node, interfaceIndex);

#ifdef ADDON_DB
    if (!alreadyDisabled)
    {
        // Record this in the interface status table
#if 0
        if (node->partitionData->statsDb->statsStatusTable->createInterfaceStatusTable)
        {
            StatsDBInterfaceStatus interfaceStatus;
            char interfaceAddrStr[100];
            NetworkIpGetInterfaceAddressString(node, interfaceIndex, interfaceAddrStr);
            interfaceStatus.m_triggeredUpdate = TRUE;
            interfaceStatus.m_address = interfaceAddrStr;
            interfaceStatus.m_interfaceEnabled =
                NetworkIpInterfaceIsEnabled(node, interfaceIndex);

            STATSDB_HandleInterfaceStatusTableInsert(node, interfaceStatus);
        }
#endif

        STATSDB_HandleInterfaceStatusTableInsert(node, TRUE, interfaceIndex);
        StatsDb* db = node->partitionData->statsDb;
        if (db != NULL && db->statsStatusTable->createNodeStatusTable &&
            db->statsNodeStatus->isActiveState)
        {
            int activeInterfaces = 0;
            for (i = 0; i < node->numberInterfaces; i++)
            {
                if (NetworkIpInterfaceIsEnabled(node, i))
                {
                    activeInterfaces++;
                }
            }
            // The node has become inactive if the last enabled interface was disabled
            if (activeInterfaces == 0)
            {
                StatsDBNodeStatus nodeStatus(node, TRUE);
                nodeStatus.m_Active = STATS_DB_Disabled;
                nodeStatus.m_ActiveStateUpdated = TRUE;
                STATSDB_HandleNodeStatusTableInsert(node, nodeStatus);
            }
        }
    }
#endif

#ifdef ENTERPRISE_LIB
    if (MAC_IsASwitch(node))
    {
        Switch_DisableInterface(node, interfaceIndex);
    }
#endif // ENTERPRISE_LIB

    // Drop all the packets from the queue which are currently there.
    while (NetworkIpOutputQueueNumberInQueue(
        node, interfaceIndex, FALSE, ALL_PRIORITIES))
    {
        Message *queuedMsg = NULL;
        MacHWAddress destAddr;

        NetworkIpOutputQueueDropPacket(node, interfaceIndex, &queuedMsg,
                                       &destAddr);
#ifdef ADDON_DB
        HandleMacDBEvents(node,
                          queuedMsg,
                          node->macData[interfaceIndex]->phyNumber,
                          interfaceIndex,
                          MAC_Drop,
                          node->macData[interfaceIndex]->macProtocol,
                          TRUE,
                          "Interface Disabled");
#endif // ADDON_DB

        MAC_NotificationOfPacketDrop(
            node,
            destAddr,
            interfaceIndex,
            queuedMsg);
    }

    MAC_InformInterfaceStatusHandler(node,
                                     interfaceIndex,
                                     MAC_INTERFACE_DISABLE);

    // Temporarily remove the static route entries which
    // use this interface
    for (i = 0; i < forwardTable->size; i++)
    {
        if ((forwardTable->row[i].protocolType == ROUTING_PROTOCOL_STATIC
            || forwardTable->row[i].protocolType ==
            ROUTING_PROTOCOL_DEFAULT)
            && forwardTable->row[i].interfaceIndex == interfaceIndex)
        {
            forwardTable->row[i].interfaceIsEnabled = FALSE;
        }
    }
    // send event to GUI for showing deactivated interface
    GUI_SendInterfaceActivateDeactivateStatus(node->nodeId,
                                              GUI_DEACTIVATE_INTERFACE,
                                              interfaceIndex);
}


// --------------------------------------------------------------------------
// FUNCTION: MAC_ProcessEnableInterface
// PURPOSE:  Enable the specific interface
// --------------------------------------------------------------------------
void MAC_ProcessEnableInterface(Node*    node,
                                Message* msg,
                                int      interfaceIndex)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    NetworkForwardingTable *forwardTable = &(ip->forwardTable);
    int i;

    MacFaultInfo* faultInfo = (MacFaultInfo*) MESSAGE_ReturnInfo(msg);

    // this condition is required to decrement the fault count
    // it decrements only in case of static and random fault
    if (faultInfo->faultType == STATIC_FAULT
            || faultInfo->faultType == RANDOM_FAULT)
    {
        node->macData[interfaceIndex]->faultCount--;

        if (node->macData[interfaceIndex]->faultCount > 0
            && !MAC_InterfaceIsEnabled(node, interfaceIndex))
        {
#ifdef ADDON_BOEINGFCS
            node->macData[interfaceIndex]->ifOperStatus = 2;
#endif
            return;
        }
    }
    if (MAC_InterfaceIsEnabled(node, interfaceIndex))
    {
        return;
    }

#ifdef ADDON_BOEINGFCS
    node->macData[interfaceIndex]->ifOperStatus = 1;
#endif
    if (DEBUG)
    {
        char time[20];
        TIME_PrintClockInSecond(node->getNodeTime(), time, node);
        printf("Making interface enable: node %u interface %d\n",
            node->nodeId, interfaceIndex);
        printf("\tCurrent time is %s\n", time);
    }

#ifdef ADDON_DB
    // if this interface is already enabled, then it should not
    // trigger a DB insert
    BOOL alreadyEnabled = NetworkIpInterfaceIsEnabled(node, interfaceIndex);
#endif

    MAC_EnableInterface(node, interfaceIndex);

#ifdef ADDON_DB
    if (!alreadyEnabled)
    {
        // Record this in the interface status table
#if 0
        if (node->partitionData->statsDb->statsStatusTable->createInterfaceStatusTable)
        {
            StatsDBInterfaceStatus interfaceStatus;
            char interfaceAddrStr[100];
            NetworkIpGetInterfaceAddressString(node, interfaceIndex, interfaceAddrStr);
            interfaceStatus.m_triggeredUpdate = TRUE;
            interfaceStatus.m_address = interfaceAddrStr;
            interfaceStatus.m_interfaceEnabled =
                NetworkIpInterfaceIsEnabled(node, interfaceIndex);

            STATSDB_HandleInterfaceStatusTableInsert(node, interfaceStatus);
        }
#endif
        STATSDB_HandleInterfaceStatusTableInsert(node, TRUE, interfaceIndex);
        StatsDb* db = node->partitionData->statsDb;
        if (db != NULL && db->statsStatusTable->createNodeStatusTable &&
            db->statsNodeStatus->isActiveState)
        {
            int i;
            int activeInterfaces = 0;
            for (i = 0; i < node->numberInterfaces; i++)
            {
                if (NetworkIpInterfaceIsEnabled(node, i))
                {
                    activeInterfaces++;
                }
            }
            // The node has become active if this is the first interface to become enabled
            if (activeInterfaces == 1)
            {
                StatsDBNodeStatus nodeStatus(node, TRUE);
                nodeStatus.m_Active = STATS_DB_Enabled;
                nodeStatus.m_ActiveStateUpdated = TRUE;
                STATSDB_HandleNodeStatusTableInsert(node, nodeStatus);
            }
        }
    }
#endif

    if (ArpIsEnable(node, interfaceIndex))
    {
        MacFaultInfo* faultInfo = (MacFaultInfo*) MESSAGE_ReturnInfo(msg);
        if (faultInfo->interfaceCardFailed)
        {
            // Before enabling the interface replace the card
            MacHWAddress* infoHW = &faultInfo->macHWAddress;

            memcpy(node->macData[interfaceIndex]->macHWAddr.byte,
                infoHW->byte, infoHW->hwLength);

            node->macData[interfaceIndex]->interfaceCardFailed = FALSE;
            ArpHandleHWInterfaceFault(node, interfaceIndex, FALSE);
        }
    }



    if (!MAC_OutputQueueIsEmpty(node, interfaceIndex))
    {
        MAC_NetworkLayerHasPacketToSend(node, interfaceIndex);
    }


    MAC_InformInterfaceStatusHandler(node,
                                     interfaceIndex,
                                     MAC_INTERFACE_ENABLE);

    // Enable the static route entries through this interface
    for (i=0; i < forwardTable->size; i++)
    {
        if ((forwardTable->row[i].protocolType == ROUTING_PROTOCOL_STATIC
            || forwardTable->row[i].protocolType ==
            ROUTING_PROTOCOL_DEFAULT)
            && forwardTable->row[i].interfaceIndex == interfaceIndex
            && forwardTable->row[i].interfaceIsEnabled == FALSE)
        {
            forwardTable->row[i].interfaceIsEnabled = TRUE;
        }
    }

#ifdef ENTERPRISE_LIB
    if (MAC_IsASwitch(node))
    {
        Switch_EnableInterface(node, interfaceIndex);
    }
#endif // ENTERPRISE_LIB
    // send event to GUI for showing deactivated interface
    GUI_SendInterfaceActivateDeactivateStatus(node->nodeId,
                                              GUI_ACTIVATE_INTERFACE,
                                              interfaceIndex);
}

// /**
// FUNCTION   :: MAC_IPv4addressIsMulticastAddress
// LAYER      :: MAC
// PURPOSE    :: Check for Multicast address or not.
// PARAMETERS ::
// + ipV4 : NodeAddress : ipV4 address
// RETURN     :: BOOL : TRUE or FALSE
// **/
BOOL MAC_IPv4addressIsMulticastAddress(
    NodeAddress ipV4)
{
    // Type D (multicast Address)
    //identified by 1110(E) of 4 MSB bits
    NodeAddress multiCastBit = 0xE;
    NodeAddress mask = mask ^ mask;
    mask = (multiCastBit ^ (ipV4>>28));
    if (mask)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}

// /**
// FUNCTION   :: MAC_TranslateMulticatIPv4AddressToMulticastMacAddress
// LAYER      :: MAC
// PURPOSE    :: Convert the Multicast ip address to multicast MAC address
// PARAMETERS ::
// + multcastAddress : NodeAddress : Multicast ip address
// + macMulticast : MacHWAddress* : Pointer to mac hardware address
// RETURN     :: void : NULL
// **/
void MAC_TranslateMulticatIPv4AddressToMulticastMacAddress(
    NodeAddress multcastAddress,
    MacHWAddress* macMulticast)
{
    unsigned char mask;
    //First 3 byte is IANA
    macMulticast->byte[5] = 0x01;
    macMulticast->byte[4] = 0x00;
    macMulticast->byte[3] = 0x5E;
    mask = mask ^ mask;
    macMulticast->byte[2] =(unsigned char)
                                      (mask | ((multcastAddress << 9)>>25));
    macMulticast->byte[1] = (unsigned char)((multcastAddress << 16)>>24);
    macMulticast->byte[0] = (unsigned char)((multcastAddress << 24)>>24);
}



// --------------------------------------------------------------------------
// FUNCTION: MAC_ProcessEvent
// PURPOSE:  Models the behavior of the MAC Layer on receiving the message
// --------------------------------------------------------------------------
void
MAC_ProcessEvent(Node *node, Message *msg)
{
    int interfaceIndex = MESSAGE_GetInstanceId(msg);
    node->enterInterface(interfaceIndex);

    int protocol = MESSAGE_GetProtocol(msg);
    clocktype faultDuration = 0;

    RandFault* randFaultPtr =
                (RandFault*) node->macData[interfaceIndex]->randFault;

    if (MESSAGE_GetEvent(msg) == MSG_MAC_StartFault)
    {
        MacFaultInfo* faultInfo = (MacFaultInfo*) MESSAGE_ReturnInfo(msg);
        FaultType     faultType = faultInfo->faultType;
        // DHCP
        AppDHCPClientFaultStart(node, interfaceIndex);
        // DHCP
#ifdef NETSNMP_INTERFACE
        node->macData[interfaceIndex]->interfaceState = INTERFACE_DOWN;
        node->macData[interfaceIndex]->interfaceDownTime = node->getNodeTime();
        if (node->notification_para != SEND_NOTRAP_NOINFORM
            && node->isSnmpEnabled)
        {
            Message   *TrapMsg = MESSAGE_Alloc(node,
                                       APP_LAYER,
                                       APP_SNMP_TRAP,
                                       MSG_SNMPV3_TRAP);
            int *TrapTypeInfo;
            MESSAGE_AddInfo(node, TrapMsg, sizeof(int), INFO_TYPE_SNMPV3);
            TrapTypeInfo = (int *)
                MESSAGE_ReturnInfo(TrapMsg,INFO_TYPE_SNMPV3);
            *TrapTypeInfo = SNMP_TRAP_LINKDOWN;
            MESSAGE_Send(node, TrapMsg, 0);
        }
#endif

#ifdef LTE_LIB
        if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_LTE ||
            node->epcData)
        {
            // LTE model does not support interface fault.
            ERROR_ReportWarning("Interface fault is not "
                    "supported for eNBs, UEs or interfaces in EPC subnet");

            MESSAGE_Free(node, msg);
            node->exitInterface();
            return;
        }
#endif

        if (faultType == RANDOM_FAULT)
        {
            ERROR_Assert(randFaultPtr,
                "Interface must have 'randFaultPtr'");

            faultDuration = randFaultPtr->randomDownTm.getRandomNumber();

            if (randFaultPtr->repetition > 0 &&
                randFaultPtr->repetition == randFaultPtr->numLinkfault)
            {
                MESSAGE_Free(node, msg);
                node->exitInterface();
                return;
            }
            randFaultPtr->numLinkfault += 1;
            randFaultPtr->totDuration += faultDuration;

            randFaultPtr->totFramesDrop +=
                NetworkIpOutputQueueNumberInQueue(
                    node, interfaceIndex, FALSE, 0);

            if (RAND_FAULT_DEBUG)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time, node);
                printf("RAND_LINK_FAULT: In NODE %u at Interface %u,"
                       " Time %s\n",node->nodeId, interfaceIndex, time);
                TIME_PrintClockInSecond(faultDuration, time);
                printf("\tStart Random Fault for %s Second\n", time);
                printf("\n\n");
            }
        }

        MAC_ProcessDisableInterface(node, msg, interfaceIndex);

        if (faultType == RANDOM_FAULT)
        {
            // change the incoming message event type
            MESSAGE_SetEvent(msg, MSG_MAC_EndFault);
            MESSAGE_Send(node, msg, faultDuration);
            // exit
            node->exitInterface();
            return;
        }
        MESSAGE_Free(node, msg);
        // exit
        node->exitInterface();
        return;
    }
    else if (MESSAGE_GetEvent(msg) == MSG_MAC_EndFault)
    {
        MacFaultInfo* faultInfo = (MacFaultInfo*) MESSAGE_ReturnInfo(msg);
        FaultType     faultType = faultInfo->faultType;
        // DHCP
        AppDHCPClientFaultEnd(node, interfaceIndex);
        // DHCP
#ifdef NETSNMP_INTERFACE
        node->macData[interfaceIndex]->interfaceState = INTERFACE_UP;
        node->macData[interfaceIndex]->interfaceUpTime = node->getNodeTime();

        if (node->notification_para != SEND_NOTRAP_NOINFORM
            && node->isSnmpEnabled)
        {
            Message   *TrapMsg = MESSAGE_Alloc(node,
                                           APP_LAYER,
                                           APP_SNMP_TRAP,
                                           MSG_SNMPV3_TRAP);

            int *TrapTypeInfo;
            MESSAGE_AddInfo(node, TrapMsg, sizeof(int), INFO_TYPE_SNMPV3);
            TrapTypeInfo = (int *)
                MESSAGE_ReturnInfo(TrapMsg,INFO_TYPE_SNMPV3);
            *TrapTypeInfo = SNMP_TRAP_LINKUP;

            MESSAGE_Send(node, TrapMsg,0);
        }
#endif

#ifdef LTE_LIB
        if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_LTE ||
            node->epcData)
        {
            // LTE model does not support interface fault.
            ERROR_ReportWarning("Interface fault is not "
                    "supported for eNBs, UEs or interfaces in EPC subnet");

            MESSAGE_Free(node, msg);
            node->exitInterface();
            return;
        }
#endif

        MAC_ProcessEnableInterface(node, msg, interfaceIndex);

        if (faultType == RANDOM_FAULT)
        {
            clocktype mTBF = 0;

            ERROR_Assert(randFaultPtr,
                "Interface must have 'randFaultPtr'");

            mTBF = randFaultPtr->randomUpTm.getRandomNumber();

            // change the incoming message event type
            MESSAGE_SetEvent(msg, MSG_MAC_StartFault);
            MESSAGE_Send(node, msg, mTBF);

            if (RAND_FAULT_DEBUG)
            {
                char time[MAX_STRING_LENGTH];
                TIME_PrintClockInSecond(node->getNodeTime(), time, node);
                printf("RAND_LINK_FAULT: In NODE %u at Interface %u,"
                       " Time %s\n",node->nodeId, interfaceIndex, time);
                TIME_PrintClockInSecond(mTBF, time);
                printf("\tRepair Random Fault for %s Second\n", time);
                printf("\n\n");
            }
            node->exitInterface();
            return;
        }

        MESSAGE_Free(node, msg);
        node->exitInterface();
        return;
    }
    else if (MESSAGE_GetEvent(msg) == MSG_MAC_StartBGTraffic
             || MESSAGE_GetEvent(msg) == MSG_MAC_EndBGTraffic)
    {
        BgTraffic_ProcessEvent(node, msg);
        node->exitInterface();
        return;
    }

#ifdef ENTERPRISE_LIB
    else
    if (protocol == MAC_PROTOCOL_MPLS)
    {
        MplsLayer(node, interfaceIndex, msg);
        node->exitInterface();
        return;
    }
    else if (protocol == MAC_SWITCH)
    {
        Switch_Layer(node, interfaceIndex, msg);
        node->exitInterface();
        return;
    }
#endif // ENTERPRISE_LIB

#ifdef ADDON_BOEINGFCS
   else if (SRW::Nexus::ConsumeMessage(protocol))
   {
     SRW::Nexus::ProcessEvent(msg);
     node->exitInterface();
     return;
   }
#endif


    /* Select the MAC protocol model, and direct it to handle the message. */

    switch (node->macData[interfaceIndex]->macProtocol)
    {
        case MAC_PROTOCOL_802_3:
        {
            Mac802_3Layer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_LINK:
        {
            LinkLayer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_SATCOM:
        {
            MacSatComLayer(node, interfaceIndex, msg);
            break;
        }
#ifdef WIRELESS_LIB
        case MAC_PROTOCOL_DOT11:
        {
            MacDot11Layer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_CSMA:
        {
            MacCsmaLayer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_MACA:
        {
            MacMacaLayer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_TDMA:
        {
            MacTdmaLayer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_ALOHA:
        {
            AlohaProcessEvent(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_GENERICMAC:
        {
            GenericMacProcessEvent(node, interfaceIndex, msg);
            break;
        }
#endif // WIRELESS_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case MAC_PROTOCOL_DOT16:
        {
            MacDot16Layer(node, interfaceIndex, msg);
            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case MAC_PROTOCOL_LTE:
        {
            LteLayer2ProcessEvent(node, interfaceIndex, msg);
            break;
        }
#endif // LTE_LIB

#ifdef SENSOR_NETWORKS_LIB
        case MAC_PROTOCOL_802_15_4:
        {
            Mac802_15_4Layer(node, interfaceIndex, msg);
            break;
        }
#endif  //SENSOR_NETWORKS_LIB

#ifdef ENTERPRISE_LIB
        case MAC_PROTOCOL_SWITCHED_ETHERNET:
        {
            SwitchedEthernetLayer(node, interfaceIndex, msg);
            break;
        }
#endif // ENTERPRISE_LIB

#ifdef CELLULAR_LIB
        case MAC_PROTOCOL_GSM:
        {
            MacGsmLayer(node, interfaceIndex, msg);
            break;
        }
#endif // CELLULAR_LIB

#ifdef WIRELESS_LIB
        case MAC_PROTOCOL_CELLULAR:
        {
            MacCellularLayer(node, interfaceIndex, msg);
            break;
        }
#endif // WIRELESS_LIB

#ifdef ALE_ASAPS_LIB
        case MAC_PROTOCOL_ALE:
        {
            MacAleLayer(
                node,
                (MacDataAle *)node->macData[interfaceIndex]->macVar, msg);
            break;
        }
#endif // ALE_ASAPS_LIB

#ifdef ADDON_LINK16
        case MAC_PROTOCOL_SPAWAR_LINK16:
        {
            MacSpawar_Link16Layer(node, interfaceIndex, msg);
            break;
        }
#endif // ADDON_LINK16

#ifdef MILITARY_RADIOS_LIB
        case MAC_PROTOCOL_TADIL_LINK11:
        {
            TadilLink11_ProcessEvent(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_TADIL_LINK16:
        {
            TadilLink16_ProcessEvent(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_FCSC_CSMA:
        {
            MacFcscCsmaLayer(node, interfaceIndex, msg);
            break;
        }
#endif // MILITARY_RADIOS_LIB
#ifdef ADDON_BOEINGFCS
        case MAC_PROTOCOL_CES_WNW_MDL:
        {
            MacWnwMiMdlLayer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_CES_WINTNCW:
        {
            MacCesWintNcwLayer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_CES_WINTHNW:
        {
            MacCesWintHnwLayer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_CES_WINTGBS:
        {
            MacCesWintGBSLayer(node, interfaceIndex, msg);
            break;
        }
    case MAC_PROTOCOL_CES_EPLRS:
     {
         if (NetworkCesIncIsEnable(node, interfaceIndex) &&
             node->macData[interfaceIndex]->macVarLink != NULL)
         {
             LinkLayer(node, interfaceIndex, msg);
             break;
         }
         else
         {
             MacCesEplrsProcessEvent(node, interfaceIndex, msg);
             break;
         }
     }
    case MAC_PROTOCOL_LINK_EPLRS:
     {
         LinkLayer(node, interfaceIndex, msg);
         break;
     }
        case MAC_PROTOCOL_CES_SINCGARS:
        {
            MacCesSincgarsProcessEvent(node, interfaceIndex, msg);
            break;
        }
#endif // ADDON_BOEINGFCS
#ifdef MODE5_LIB
        case MAC_PROTOCOL_MODE5:
        {
          MESSAGE_Free(node, msg);
        } break;
#endif
#ifdef SATELLITE_LIB
        case MAC_PROTOCOL_ANE:
        {
            MacAneLayer(node, interfaceIndex, msg);
            break;
        }
        case MAC_PROTOCOL_SATELLITE_BENTPIPE:
        {
            MacSatelliteBentpipeProcessEvent(node,
                                             interfaceIndex,
                                             msg);

            break;
        }
#endif // SATELLITE_LIB

#ifdef CYBER_LIB
        case MAC_PROTOCOL_WORMHOLE:
        {
            MacWormholeLayer(node, interfaceIndex, msg);
            break;
        }
#endif // CYBER_LIB

//InsertPatch LAYER_FUNCTION
        default:
            ERROR_ReportError("Unknown or disabled MAC protocol");
            break;
    }//switch//

    node->exitInterface();
}

#ifdef ENTERPRISE_LIB
void MAC_SwitchHasPacketToSend(Node* node, int interfaceIndex)
{
    if (!MAC_InterfaceIsEnabled(node, interfaceIndex))
    {
        Message* msg = NULL;

        Switch_QueueDropPacketForInterface(
            node,
            interfaceIndex,
            &msg);
#ifdef ADDON_DB
        HandleMacDBEvents(node,
                          msg,
                          node->macData[interfaceIndex]->phyNumber,
                          interfaceIndex,
                          MAC_Drop,
                          node->macData[interfaceIndex]->macProtocol,
                          TRUE,
                          "Interface Disabled");
#endif // ADDON_DB

        MESSAGE_Free(node, msg);
    }
    else
    {
        switch (node->macData[interfaceIndex]->macProtocol)
        {
            case MAC_PROTOCOL_802_3:
            {
                (*node->macData[interfaceIndex]->sendFrameFn)
                    (node, interfaceIndex);
                break;
            }
            case MAC_PROTOCOL_LINK:
            {
                (*node->macData[interfaceIndex]->sendFrameFn)
                    (node, interfaceIndex);
                break;
            }
            default:
            {
                ERROR_Assert(FALSE,
                    "MAC_SwitchHasPacketToSend: "
                    "Switch supports 802.3 and wired link.\n");
                break;
            }
        }
    }
}
#endif // ENTERPRISE_LIB


// --------------------------------------------------------------------------
// FUNCTION: MAC_IsLayer2Device
// PURPOSE:  If the node is a Layer2Device, i.e., without Layer 3 routing
// PARAMETERS ::
// + node : Node * : the node
// + interfaceIndex : int : the interface #
// RETURN : BOOL : TRUE if yes; otherwise FALSE
// --------------------------------------------------------------------------
static BOOL MAC_IsLayer2Device(Node *node, int interfaceIndex)
{
    if (MAC_IsASwitch(node))
    {
        return TRUE;
    }
#ifdef CYBER_LIB
    // Wormholes in TRANSPARENT mode are layer 2 devices
    if (node->macData[interfaceIndex]->isWormhole == TRUE)
    {
        return TRUE;
    }
#endif // CYBER_LIB
#ifdef MILITARY_RADIOS_LIB
    MAC_PROTOCOL macproto = node->macData[interfaceIndex]->macProtocol;
    if (macproto == MAC_PROTOCOL_TADIL_LINK11 ||
        macproto == MAC_PROTOCOL_TADIL_LINK16)
    {
        return TRUE;
    }
#endif // MILITARY_RADIOS_LIB
    return FALSE;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_NetworkLayerHasPacketToSend
// PURPOSE:  Handles packets from the network layer when the network queue
//           is empty.
// PARAMETERS ::
// + node : Node * : the node
// + interfaceIndex : int : the interface #
// RETURN : void
// --------------------------------------------------------------------------
void
MAC_NetworkLayerHasPacketToSend(Node *node, int interfaceIndex)
{
    /* Select the MAC protocol model, and direct it to send/buffer the
       packet. */
    node->enterInterface(interfaceIndex);

    if (MAC_IsLayer2Device(node, interfaceIndex))
    {
        // silently discard the packet
        NodeAddress nextHopAddress = 0;
        MacHWAddress macAddr;
        Message* msg = NULL;
        nextHopAddress = NetworkIpOutputQueueDropPacket(
                            node,
                            interfaceIndex,
                            &msg,&macAddr);
#ifdef ADDON_DB
        HandleMacDBEvents(node,
                          msg,
                          node->macData[interfaceIndex]->phyNumber,
                          interfaceIndex,
                          MAC_Drop,
                          node->macData[interfaceIndex]->macProtocol,
                          TRUE,
                          "Layer2 Device",
                          FALSE);
#endif // ADDON_DB

        MESSAGE_Free(node, msg);
        node->exitInterface();
        return;
    }

    if (DEBUG)
    {
        char clockStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr, node);
        printf("Node %d received packet to send from network layer "
               "at time %s\n",
                node->nodeId, clockStr);
    }

    if (!MAC_InterfaceIsEnabled(node, interfaceIndex))
    {
        NodeAddress nextHopAddress = 0;
        MacHWAddress macAddr;
        Message* msg = NULL;
#ifdef ADDON_BOEINGFCS
        if (NetworkCesIncSincgarsActiveOnInterface(node, interfaceIndex))
        {
            nextHopAddress = NetworkCesIncSincgarsOutputQueueDropPacket(node,
                                                           interfaceIndex,
                                                           &msg);
            MAC_FourByteMacAddressToVariableHWAddress(node,interfaceIndex,
                                                  &macAddr,nextHopAddress);
        }
        else
        {
#endif
#ifdef NETSNMP_INTERFACE
        int localPriority = -1;

        int routingProtocolType = NETWORK_PROTOCOL_IP;
        BOOL peek = NetworkIpOutputQueueTopPacket(node, interfaceIndex, &msg,
                &nextHopAddress, &macAddr, &routingProtocolType,
               &localPriority);
#endif

        nextHopAddress = NetworkIpOutputQueueDropPacket(
                         node,
                         interfaceIndex,
                         &msg,&macAddr);
#ifdef ADDON_BOEINGFCS
        }
#endif
#ifdef ADDON_DB
        HandleMacDBEvents(node,
                          msg,
                          node->macData[interfaceIndex]->phyNumber,
                          interfaceIndex,
                          MAC_Drop,
                          node->macData[interfaceIndex]->macProtocol,
                          TRUE,
                          "Interface Disabled");
#endif // ADDON_DB

        MAC_NotificationOfPacketDrop(
            node,
            macAddr,
            interfaceIndex,
            msg);

        node->exitInterface();
        return;
    }

    switch (node->macData[interfaceIndex]->macProtocol)
    {
        case MAC_PROTOCOL_LINK:
        {
            (*node->macData[interfaceIndex]->sendFrameFn)
                (node, interfaceIndex);
            break;
        }
        case MAC_PROTOCOL_802_3:
        {
            (*node->macData[interfaceIndex]->sendFrameFn)
                (node, interfaceIndex);
            break;
        }
        case MAC_PROTOCOL_SATCOM:
        {
            MacSatComNetworkLayerHasPacketToSend(
                node,
                (MacSatComData *)
                node->macData[interfaceIndex]->macVar);

            break;
        }

#ifdef WIRELESS_LIB
        case MAC_PROTOCOL_DOT11:
        {
            MacDot11NetworkLayerHasPacketToSend(
                node, (MacDataDot11 *) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_CSMA:
        {
            MacCsmaNetworkLayerHasPacketToSend(
                node, (MacDataCsma *) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_MACA:
        {
            MacMacaNetworkLayerHasPacketToSend(
                node, (MacDataMaca *) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_TDMA:
        {
            MacTdmaNetworkLayerHasPacketToSend(
                node, (MacDataTdma *) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_ALOHA:
        {
            MacAlohaNetworkLayerHasPacketToSend(
                node,
                (AlohaData *) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_GENERICMAC:
        {
            MacGenericMacNetworkLayerHasPacketToSend(
                node,
                (GenericMacData *) node->macData[interfaceIndex]->macVar);
            break;
        }
#endif // WIRELESS_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case MAC_PROTOCOL_DOT16:
        {
            MacDot16NetworkLayerHasPacketToSend(
                node, (MacDataDot16 *) node->macData[interfaceIndex]->macVar);
            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case MAC_PROTOCOL_LTE:
        {
            LteLayer2UpperLayerHasPacketToSend(
                node,
                interfaceIndex,
                (Layer2DataLte*)node->macData[interfaceIndex]->macVar);
            break;
        }
#endif // LTE_LIB

#ifdef SENSOR_NETWORKS_LIB
        case MAC_PROTOCOL_802_15_4:
        {
            Mac802_15_4NetworkLayerHasPacketToSend(
                    node, (MacData802_15_4* )
                    node->macData[interfaceIndex]->macVar);
            break;
        }
#endif  //SENSOR_NETWORKS_LIB

#ifdef MILITARY_RADIOS_LIB
        case MAC_PROTOCOL_TADIL_LINK11:
        {
            TadilLink11NetworkLayerHasPacketToSend(
                node,
                (MacDataLink11 *) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_TADIL_LINK16:
        {
            break;
        }
        case MAC_PROTOCOL_FCSC_CSMA:
        {
            MacFcscCsmaNetworkLayerHasPacketToSend(
                node,
                (MacDataFcscCsma *) node->macData[interfaceIndex]->macVar);
            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef ENTERPRISE_LIB
        case MAC_PROTOCOL_SWITCHED_ETHERNET:
        {
            SwitchedEthernetNetworkLayerHasPacketToSend(
                node,
                (MacSwitchedEthernetData *)
                node->macData[interfaceIndex]->macVar);

            break;
        }
#endif // ENTERPRISE_LIB

#ifdef CELLULAR_LIB
        case MAC_PROTOCOL_GSM:
        {
            MacGsmNetworkLayerHasPacketToSend(
                node, (MacDataGsm *) node->macData[interfaceIndex]->macVar);
            break;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case MAC_PROTOCOL_CELLULAR:
        {
            MacCellularNetworkLayerHasPacketToSend(node, interfaceIndex, 
                                                   (MacCellularData *) node->macData[interfaceIndex]->macVar);
            break;
        }
#endif //WIRELESS_LIB

#ifdef ALE_ASAPS_LIB
        case MAC_PROTOCOL_ALE:
        {
            MacAleNetworkLayerHasPacketToSend(
                node, (MacDataAle *) node->macData[interfaceIndex]->macVar);
            break;
        }
#endif // ALE_ASAPS_LIB

#ifdef ADDON_LINK16
        case MAC_PROTOCOL_SPAWAR_LINK16:
        {
            MacSpawar_Link16NetworkLayerHasPacketToSend(
              node, (MacDataSpawar_Link16 *) node->macData[interfaceIndex]->macVar);
            break;
        }
#endif // ADDON_LINK16
#ifdef MODE5_LIB
        case MAC_PROTOCOL_MODE5:
        {
          Message* msg(NULL);
          MacHWAddress destHWAddr;
          int networkType;
          TosType priority;

          MAC_OutputQueueDequeuePacket(node, interfaceIndex, &msg, &destHWAddr, &networkType,
                                       &priority);

          MESSAGE_Free(node, msg);
        } break;
#endif
#ifdef SATELLITE_LIB
        case MAC_PROTOCOL_SATTSM:
        case MAC_PROTOCOL_ANE:
        {
            MacAneNetworkLayerHasPacketToSend(node,
                (MacAneState*)node->macData[interfaceIndex]->macVar);

            break;
        }
        case MAC_PROTOCOL_SATELLITE_BENTPIPE: {
            MacSatelliteBentpipeNetworkLayerHasPacketToSend(
                node,
                (MacSatelliteBentpipeState*)node->
                    macData[interfaceIndex]->macVar);

            break;
        }
#endif // SATELLITE_LIB

#ifdef CYBER_LIB
        case MAC_PROTOCOL_WORMHOLE:
        {
#undef PARTICIPANT
#ifdef PARTICIPANT
            MacWormholeNetworkLayerHasPacketToSend(
                node,
                (MacWormholeData *)
                node->macData[interfaceIndex]->macVar);
#endif // PARTICIPANT
            break;
        }
#endif // CYBER_LIB
#ifdef ADDON_BOEINGFCS
        case MAC_PROTOCOL_CES_WNW_MDL:
        {
            MacWnwMiMdlNetworkLayerHasPacketToSend(
                 node,
                 (MacCesDataUsapDBA*) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_CES_WINTNCW:
        {
            MacCesWintNcwNetworkLayerHasPacketToSend(
                node,
                (MacCesWintNcwData *)
                node->macData[interfaceIndex]->macVar);

            break;
        }
        case MAC_PROTOCOL_CES_WINTHNW:
        {
            MacCesWintHnwNetworkLayerHasPacketToSend(
                    node, (MacCesWintHnwData *) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_CES_WINTGBS:
        {
            MacCesWintGBSNetworkLayerHasPacketToSend(
                    node,
            (MacCesWintGBSData*) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_CES_EPLRS:
        {
            //j.h - if EPLRS is enabled, queue packets in
            // needline queue according to priorities
            MacCesEplrsNetworkLayerHasPacketToSend(node, interfaceIndex);
            break;
            }
        case MAC_PROTOCOL_CES_SINCGARS:
            {
            MacCesSincgarsNetworkLayerHasPacketToSend(
                    node,
                (MacCesSincgarsData *) node->macData[interfaceIndex]->macVar);
            break;
        }
        case MAC_PROTOCOL_CES_SRW_PORT:
        {
            SRW::Nexus::NetworkRTS(node,
                                   node->macData[interfaceIndex]->macVar);
            break;
        }
#endif // ADDON_BOEINGFCS

//InsertPatch NETWORK_SEND
        default:
            ERROR_Assert(FALSE, "Unknown MAC protocol"); abort();
            break;
    }//switch//
    node->exitInterface();
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_ReceivePacketFromPhy
// PURPOSE:  Handles packets that was just received from the physical medium.
// --------------------------------------------------------------------------
void
MAC_ReceivePacketFromPhy(
    Node *node,
    int interfaceIndex,
    Message *packet,
    int phyNum)
{
#ifdef CYBER_LIB
    /* Drop the packet at the mac layer if the mac protocl is not
    supported for app jamming */
    JamType *infoPtr = (JamType *) MESSAGE_ReturnInfo(packet,INFO_TYPE_JAM);
    if (infoPtr != NULL)
    {
        switch(*infoPtr)
        {
            case JAM_TYPE_APP:
            switch(node->macData[interfaceIndex]->macProtocol)
            {
                case MAC_PROTOCOL_GSM:
                case MAC_PROTOCOL_802_15_4:
                case MAC_PROTOCOL_TDMA:
                case MAC_PROTOCOL_CSMA:
                case MAC_PROTOCOL_GENERICMAC:
                case MAC_PROTOCOL_DOT11:
                    break;
                default:
                    MESSAGE_Free(node, packet);
                    return;
            }
            default:
                break;
        }
    }
#endif // CYBER_LIB
    if (DEBUG >= 2)
    {
        char clockStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("Node %d (time %s) received a packet of size %d "
               "originated from node %d at time %15" TYPES_64BITFMT
               "d from PHY\n",
               node->nodeId, clockStr, MESSAGE_ReturnPacketSize(packet),
               packet->originatingNodeId, packet->packetCreationTime);
    }
    if (!MAC_InterfaceIsEnabled(node, interfaceIndex))
    {
        if (node->macData[interfaceIndex]->macProtocol ==
            MAC_PROTOCOL_CELLULAR)
        {
            MESSAGE_FreeList(node, packet);
        }
        else
        {
            MESSAGE_Free(node, packet);
        }
        return;
    }

#if defined(ADDON_BOEINGFCS)
    // FOR MIBS: IfInOctets.

    int realPktSize = MESSAGE_ReturnPacketSize(packet);

    if (packet->isPacked)
    {
        realPktSize = packet->actualPktSize;
    }
    node->macData[interfaceIndex]->ifInOctets += realPktSize;
    node->macData[interfaceIndex]->ifHCInOctets += realPktSize;
#endif

#ifdef CYBER_LIB
    if (node->macData[interfaceIndex]->isWormhole == TRUE)
    {
        // Affect tunneling
        MacWormholeReceivePacketFromPhy(
            node,
            (MacWormholeData*)node->macData[interfaceIndex]->macVar,
            packet);

        // Affect replaying
        MacWormholeReplayReceivePacketFromPhy(
            node,
            (MacWormholeData*)node->macData[interfaceIndex]->macVar,
            packet);

        return;
    }
#endif // CYBER_LIB
#ifdef ADDON_DB
    HandleMacDBConnectivity(node, interfaceIndex, packet, MAC_ReceiveFromPhy);
#endif
    switch (node->macData[interfaceIndex]->macProtocol)
    {
#ifdef WIRELESS_LIB
        case MAC_PROTOCOL_DOT11: {
            MacDot11ReceivePacketFromPhy(
                node, (MacDataDot11*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
        case MAC_PROTOCOL_CSMA:
        {
            MacCsmaReceivePacketFromPhy(
                node,
                (MacDataCsma*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
        case MAC_PROTOCOL_MACA:
        {
            MacMacaReceivePacketFromPhy(
                node,
                (MacDataMaca*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
        case MAC_PROTOCOL_TDMA:
        {
            MacTdmaReceivePacketFromPhy(
                node, (MacDataTdma*)node->macData[interfaceIndex]->macVar,
                packet);
           break;
        }
        case MAC_PROTOCOL_ALOHA:
        {
            MacAlohaReceivePacketFromPhy(
                node,
                (AlohaData *) node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
        case MAC_PROTOCOL_GENERICMAC:
        {
            MacGenericMacReceivePacketFromPhy(
                node,
                (GenericMacData *) node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
#endif // WIRELESS_LIB

#ifdef CELLULAR_LIB
        case MAC_PROTOCOL_GSM:
        {
            MacGsmReceivePacketFromPhy(
                node, (MacDataGsm*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case MAC_PROTOCOL_CELLULAR:
        {
            MacCellularReceivePacketFromPhy(node, interfaceIndex, packet);
            break;
        }
#endif //WIRELESS_LIB

#ifdef ALE_ASAPS_LIB
        case MAC_PROTOCOL_ALE:
        {
            MacAleReceivePacketFromPhy(
                node,
                (MacDataAle*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
#endif // ALE_ASAPS_LIB

#ifdef ADDON_BOEINGFCS
     case MAC_PROTOCOL_CES_WNW_MDL:
     {
         MacWnwMiMdlReceivePacketFromPhy(
             node,
             (MacCesDataUsapDBA*)node->macData[interfaceIndex]->macVar,
             packet);
         break;
     }
     case MAC_PROTOCOL_CES_WINTHNW: {
         MacCesWintHnwReceivePacketFromPhy(
                 node, (MacCesWintHnwData*)node->macData[interfaceIndex]->macVar, packet);
         break;
     }
    case MAC_PROTOCOL_CES_EPLRS:
     {
          MacCesEplrsReceivePacketFromPhy(
              node,
              (MacCesEplrsData *) node->macData[interfaceIndex]->macVar,
              packet);
          break;
     }
    case MAC_PROTOCOL_CES_SINCGARS:
     {
          MacCesSincgarsReceivePacketFromPhy(
              node,
              (MacCesSincgarsData *) node->macData[interfaceIndex]->macVar,
              packet);
          break;
     }
    case MAC_PROTOCOL_CES_SIP:
     {
         CesSincgarsSipHandlePacketFromPhy(node,
             interfaceIndex,
             packet);
        break;
     }
    case MAC_PROTOCOL_CES_SRW_PORT:
    {
        SRW::Nexus::ReceivePacketFromPhy(node,
                                         node->macData[interfaceIndex]->macVar,
                                         packet,
                                         phyNum);
        break;
    }

#endif // ADDON_BOEINGFCS

#ifdef SATELLITE_LIB
        case MAC_PROTOCOL_SATELLITE_BENTPIPE: {
            MacSatelliteBentpipeReceivePacketFromPhy(
                node,
                (MacSatelliteBentpipeState*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
#endif // SATELLITE_LIB

#ifdef MILITARY_RADIOS_LIB
        case MAC_PROTOCOL_TADIL_LINK11: {
            TadilLink11ReceivePacketFromPhy(
                node,
                (MacDataLink11*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
        case MAC_PROTOCOL_TADIL_LINK16: {
            TadilLink16ReceivePacketFromPhy(
                node,
                (MacDataLink16*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
#endif  //MILITARY_RADIOS_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case MAC_PROTOCOL_DOT16: {
            MacDot16ReceivePacketFromPhy(
                node, (MacDataDot16*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case MAC_PROTOCOL_LTE: {
            LteLayer2ReceivePacketFromPhy(
                node,
                interfaceIndex,
                (Layer2DataLte*)node->macData[interfaceIndex]->macVar,
                packet);
            break;
        }
#endif // LTE_LIB

#ifdef SENSOR_NETWORKS_LIB
        case MAC_PROTOCOL_802_15_4: {
            Mac802_15_4ReceivePacketFromPhy(
                    node, (MacData802_15_4*)
                    node->macData[interfaceIndex]->macVar, packet);
            break;
        }
#endif  //SENSOR_NETWORKS_LIB

//InsertPatch RECEIVE_PHY
        default:
            ERROR_ReportError("Unknown or disabled MAC protocol");
            break;
    }//switch//
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_ManagementRequest
// PURPOSE: Dispatches network management requests into MAC layer
// --------------------------------------------------------------------------
void
MAC_ManagementRequest(
    Node *node,
    int interfaceIndex,
    ManagementRequest *req,
    ManagementResponse *resp)
{
    assert(req != NULL && resp != NULL); // references...one day

    switch (node->macData[interfaceIndex]->macProtocol) {
#ifdef ADDON_BOEINGFCS
        case MAC_PROTOCOL_CES_WNW_MDL:
            MacWnwMiMdlManagementRequest(
                node,
                interfaceIndex,
                req,
                resp);
            break;
#endif // ADDON_BOEINGFCS
        default:
            resp->result = ManagementResponseUnsupported;
            resp->data = 0;
            break;
    }
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_ReceivePhyStatusChangeNotification
// PURPOSE:  Handles status changes from the physical layer.
//
// Note from the kernel team.  It's really really unsafe to pass along
// the potentialIncomingPacket, because until it becomes an actual
// incoming packet, it's shared by all the nodes on the channel.
// --------------------------------------------------------------------------
void
MAC_ReceivePhyStatusChangeNotification(
    Node *node,
    int interfaceIndex,
    PhyStatusType oldPhyStatus,
    PhyStatusType newPhyStatus,
    clocktype receiveDuration,
    const Message *potentialIncomingPacket,
    int phyNum)
{
    switch (node->macData[interfaceIndex]->macProtocol)
    {
#ifdef WIRELESS_LIB
        case MAC_PROTOCOL_DOT11:{
            MacDot11ReceivePhyStatusChangeNotification(
                node,
                (MacDataDot11*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus,
                receiveDuration,
                potentialIncomingPacket);
            break;
        }
        case MAC_PROTOCOL_CSMA:
        {
            MacCsmaReceivePhyStatusChangeNotification(
                node,
                (MacDataCsma*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
            break;
        }
        case MAC_PROTOCOL_MACA:
        {
            MacMacaReceivePhyStatusChangeNotification(
                node,
                (MacDataMaca*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
            break;
        }
        case MAC_PROTOCOL_TDMA:
        {
            MacTdmaReceivePhyStatusChangeNotification(
                node,
                (MacDataTdma*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
            break;
        }
        case MAC_PROTOCOL_ALOHA:
        {
            MacAlohaReceivePhyStatusChangeNotification(
                node,
                (AlohaData *) node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
            break;
        }
        case MAC_PROTOCOL_GENERICMAC:
        {
            MacGenericMacReceivePhyStatusChangeNotification(
                node,
                (GenericMacData *) node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
            break;
        }
#endif // WIRELESS_LIB

#ifdef CELLULAR_LIB
        case MAC_PROTOCOL_GSM:
        {
            MacGsmReceivePhyStatusChangeNotification(
                node,
                 (MacDataGsm*)node->macData[interfaceIndex]->macVar,
                 oldPhyStatus,
                 newPhyStatus);
            break;
        }
#endif // CELLULAR_LIB

#ifdef WIRELESS_LIB
        case MAC_PROTOCOL_CELLULAR:
        {
            break;
        }
#endif //WIRELESS_LIB

#ifdef ALE_ASAPS_LIB
        case MAC_PROTOCOL_ALE:
        {
            MacAleReceivePhyStatusChangeNotification(
                node,
                (MacDataAle*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus,
                potentialIncomingPacket);
            break;
        }
#endif // ALE_ASAPS_LIB

#ifdef ADDON_LINK16
        case MAC_PROTOCOL_SPAWAR_LINK16:
        {
            MacSpawar_Link16ReceivePhyStatusChangeNotification(
                node,
                (MacDataSpawar_Link16*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
            break;
        }
#endif // ADDON_LINK16

#ifdef MILITARY_RADIOS_LIB
        case MAC_PROTOCOL_TADIL_LINK11:
        {
            TadilLink11ReceivePhyStatusChangeNotification(
                node,
                (MacDataLink11*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
            break;
        }
        case MAC_PROTOCOL_TADIL_LINK16:
        {
            TadilLink16ReceivePhyStatusChangeNotification(
                node,
                (MacDataLink16*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef ADDON_BOEINGFCS
     case MAC_PROTOCOL_CES_WNW_MDL:
     {
         MacWnwMiMdlReceivePhyStatusChangeNotification(
             node,
             (MacCesDataUsapDBA*)node->macData[interfaceIndex]->macVar,
             oldPhyStatus,
             newPhyStatus);
         break;
     }
     case MAC_PROTOCOL_CES_WINTHNW:{
         MacCesWintHnwReceivePhyStatusChangeNotification(
                node,
                (MacCesWintHnwData*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
         break;
     }
    case MAC_PROTOCOL_CES_EPLRS:
     {
          MacCesEplrsReceivePhyStatusChangeNotification(
              node,
              (MacCesEplrsData *) node->macData[interfaceIndex]->macVar,
              oldPhyStatus,
              newPhyStatus);
          break;
     }
    case MAC_PROTOCOL_CES_SINCGARS:
     {
          MacCesSincgarsReceivePhyStatusChangeNotification(
              node,
              (MacCesSincgarsData *) node->macData[interfaceIndex]->macVar,
              oldPhyStatus,
              newPhyStatus);
          break;
     }
    case MAC_PROTOCOL_CES_SIP:
     {
          //dont do nothing
          break;
     }
     case MAC_PROTOCOL_CES_SRW_PORT:
     {
         SRW::Nexus::ReceivePhyStatusUpdate(node,
                                            node->macData[interfaceIndex]->macVar,
                                            oldPhyStatus,
                                            newPhyStatus,
                                            phyNum);
         break;
     }
#endif // ADDON_BOEINGFCS

#ifdef SATELLITE_LIB
     case MAC_PROTOCOL_SATELLITE_BENTPIPE: {
         MacSatelliteBentpipeReceivePhyStatusChangeNotification(
            node,
           (MacSatelliteBentpipeState*)node->macData[interfaceIndex]->macVar,
            oldPhyStatus, newPhyStatus);
         break;
     }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case MAC_PROTOCOL_DOT16:{
            MacDot16ReceivePhyStatusChangeNotification(
                node,
                (MacDataDot16*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus,
                receiveDuration);
            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case MAC_PROTOCOL_LTE:{
            // TODO : Do not support at Phase 1
            break;
        }
#endif // LTE_LIB

#ifdef SENSOR_NETWORKS_LIB
        case MAC_PROTOCOL_802_15_4: {
            Mac802_15_4ReceivePhyStatusChangeNotification(
                node,
                (MacData802_15_4*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus,
                receiveDuration);
            break;
        }
#endif  //SENSOR_NETWORKS_LIBS

#ifdef CYBER_LIB
        case MAC_PROTOCOL_WORMHOLE:
        {
            MacWormholeReplayReceivePhyStatusChangeNotification(
                node,
                (MacWormholeData*)node->macData[interfaceIndex]->macVar,
                oldPhyStatus,
                newPhyStatus);
            break;
        }
#endif // CYBER_LIB

//InsertPatch PHY_STATUS
        default:
            ERROR_ReportError("Unknown or disabled MAC protocol");
            break;
    }//switch//
}


// --------------------------------------------------------------------------
// FUNCTION: MAC_RunTimeStat
// PURPOSE:  Processes runtime statistics.
// --------------------------------------------------------------------------
void MAC_RunTimeStat(Node *node)
{
    int interfaceIndex;

    for (interfaceIndex = 0;
         interfaceIndex < node->numberInterfaces;
         interfaceIndex++)
    {
        node->enterInterface(interfaceIndex);
        /* Select the MAC protocol model and finalize it. */

        if (node->macData[interfaceIndex])
        {
            switch (node->macData[interfaceIndex]->macProtocol)
            {
#ifdef WIRELESS_LIB
                case MAC_PROTOCOL_DOT11:
                {
                    //Runtimestat function goes here
                    break;
                }
                case MAC_PROTOCOL_802_11:
                {
                    //Runtime stat function goes here
                    break;
                }
                case MAC_PROTOCOL_CSMA:
                {
                    //Runtime stat function goes here
                    break;
                }
                case MAC_PROTOCOL_MACA:
                {
                    //Runtime stat function goes here
                    break;
                }
                case MAC_PROTOCOL_ALOHA:
                {
                    AlohaRunTimeStat(
                        node,
                        (AlohaData *) node->macData[interfaceIndex]->macVar);
                }
                case MAC_PROTOCOL_GENERICMAC:
                {
                    GenericMacRunTimeStat(
                        node,
                        (GenericMacData *) node->
                            macData[interfaceIndex]->macVar);
                }
#endif // WIRELESS_LIB
                case MAC_PROTOCOL_802_3:
                {
                    //Runtime stat function goes here
                    break;
                }
                case MAC_PROTOCOL_SWITCHED_ETHERNET:
                {
                    //Runtime stat function goes here
                    break;
                }
                case MAC_PROTOCOL_SATCOM:
                {
                    //Runtime stat function goes here
                    break;
                }
                case MAC_PROTOCOL_LINK:
                {
                    //Runtime stat function goes here
                    break;
                }
#ifdef MODE5_LIB
                case MAC_PROTOCOL_MODE5: { ; } break;
#endif
#ifdef SATELLITE_LIB
                case MAC_PROTOCOL_ANE:
                {
                    MacAneRunTimeStat(node,
                        (MacAneState*)
                        node->macData[interfaceIndex]->macVar);
                    break;
                }
                case MAC_PROTOCOL_SATELLITE_BENTPIPE:
                {
                    MacSatelliteBentpipeRunTimeStat(node,
                                                    interfaceIndex);
                    break;
                }
#endif // SATELLITE_LIB

#ifdef CYBER_LIB
                case MAC_PROTOCOL_WORMHOLE:
                {
                    //Runtime stat function goes here
                    break;
                }
#endif // CYBER_LIB
#ifdef ADDON_BOEINGFCS
     case MAC_PROTOCOL_CES_WNW_MDL:
     {
         //Runtimestat function goes here
         break;
     }
     case MAC_PROTOCOL_CES_WINTNCW:
     {
         //Runtime stat function goes here
         break;
     }
    case MAC_PROTOCOL_CES_WINTHNW:
    {
        //Runtimestat function goes here
        break;
    }
    case MAC_PROTOCOL_CES_WINTGBS:
    {
        //Runtimestat function goes here
        break;
    }
    case MAC_PROTOCOL_CES_EPLRS:
    {
        MacCesEplrsRunTimeStat(
            node,
            (MacCesEplrsData *) node->
            macData[interfaceIndex]->macVar);
        break;
    }
    case MAC_PROTOCOL_CES_SINCGARS:
    {
        MacCesSincgarsRunTimeStat(
            node,
            (MacCesSincgarsData *) node->
            macData[interfaceIndex]->macVar);
         break;
    }
    case MAC_PROTOCOL_CES_SRW_PORT:
    {
      SRW::Nexus::RunTimeStat(node,
                              node->macData[interfaceIndex]->macVar);
      break;
    }
#endif // ADDON_BOEINGFCS

//InsertPatch MAC_STATS
                default:
                    break;
            }
        }//switch//
        node->exitInterface();
    }//for//
}



// --------------------------------------------------------------------------
// FUNCTION: MAC_Finalize
// PURPOSE: Called at the end of simulation to collect the results of
//          the simulation of the MAC Layer.
// --------------------------------------------------------------------------
void
MAC_Finalize(Node *node)
{
    int interfaceIndex;

    for (interfaceIndex = 0;
         interfaceIndex < node->numberInterfaces;
         interfaceIndex++)
    {
         node->enterInterface(interfaceIndex);


#ifdef ADDON_BOEINGFCS
        if (node->macData[interfaceIndex] &&
            (node->macData[interfaceIndex]->macProtocol ==
                MAC_PROTOCOL_CES_WNW_MDL))
        {
        LinkCesWnwAdaptationFinalize(node,interfaceIndex);
        }
#endif

        /* Select the MAC protocol model and finalize it. */

        if (node->macData[interfaceIndex])
        {
            switch (node->macData[interfaceIndex]->macProtocol)
            {
                case MAC_PROTOCOL_LINK:
                {
                    LinkFinalize(node, interfaceIndex);
                    break;
                }
                case MAC_PROTOCOL_802_3:
                {
                    Mac802_3Finalize(node, interfaceIndex);
                    break;
                }
                case MAC_PROTOCOL_SATCOM:
                {
                    MacSatComFinalize(node, interfaceIndex);
                    break;
                }
#ifdef WIRELESS_LIB
                case MAC_PROTOCOL_DOT11:
                {
                    MacDot11Finalize(node, interfaceIndex);
#ifdef CYBER_LIB
                    if (node->macData[interfaceIndex]->encryptionVar != NULL)
                    {
                        MAC_SECURITY* secType = (MAC_SECURITY*)
                            node->macData[interfaceIndex]->encryptionVar;
                        if (*secType == MAC_SECURITY_WEP)
                        {
                            WEPFinalize(node, interfaceIndex);
                        } else if (*secType == MAC_SECURITY_CCMP)
                        {
                            CCMPFinalize(node, interfaceIndex);
                        }
                        else
                        {
                            ERROR_Assert(FALSE, "Unknown Security Protocol");
                        }
                    }
#endif // CYBER_LIB
                    break;
                }
                case MAC_PROTOCOL_CSMA:
                {
                    MacCsmaFinalize(node, interfaceIndex);
                    break;
                }
                case MAC_PROTOCOL_TDMA:
                {
                    MacTdmaFinalize(node, interfaceIndex);
                    break;
                }
                case MAC_PROTOCOL_MACA:
                {
                    MacMacaFinalize(node, interfaceIndex);
                    break;
                }
                case MAC_PROTOCOL_ALOHA:
                {
                    AlohaFinalize(node, interfaceIndex);
                    break;
                }
                case MAC_PROTOCOL_GENERICMAC:
                {
                    GenericMacFinalize(node, interfaceIndex);
                    break;
                }
#endif // WIRELESS_LIB
#ifdef ADDON_BOEINGFCS
     case MAC_PROTOCOL_CES_WNW_MDL:
     {
         MacWnwMiMdlFinalize(node, interfaceIndex);
         break;
     }
     case MAC_PROTOCOL_CES_WINTNCW:
     {
         MacCesWintNcwFinalize(node, interfaceIndex);
         break;
     }
    case MAC_PROTOCOL_CES_WINTHNW:
    {
        MacCesWintHnwFinalize(node, interfaceIndex);
        break;
    }
    case MAC_PROTOCOL_CES_WINTGBS:
    {
        MacCesWintGBSFinalize(node, interfaceIndex);
        break;
    }
    case MAC_PROTOCOL_CES_EPLRS:
    {
        MacCesEplrsFinalize(node, interfaceIndex);
        break;
    }
    case MAC_PROTOCOL_CES_SINCGARS:
    {
        MacCesSincgarsFinalize(node, interfaceIndex);
        break;
    }
    case MAC_PROTOCOL_CES_SIP:
    case MAC_PROTOCOL_LINK_EPLRS:
    {
        //Do nothing
        break;
    }
                 case MAC_PROTOCOL_CES_SRW_PORT:
                 {
                    SRW::Nexus::Finalize(node, interfaceIndex);
                    break;
                 }
#endif // ADDON_BOEINGFCS
#ifdef MILITARY_RADIOS_LIB
                case MAC_PROTOCOL_FCSC_CSMA:
                {
                    MacFcscCsmaFinalize(node, interfaceIndex);
                    break;
                }
                case MAC_PROTOCOL_TADIL_LINK11:
                {
                    TadilLink11Finalize(node, interfaceIndex);
                    break;
                }
                case MAC_PROTOCOL_TADIL_LINK16:
                {
                    TadilLink16Finalize(node, interfaceIndex);
                    break;
                }
#endif //MILITARY_RADIOS_LIB
#ifdef ENTERPRISE_LIB
                case MAC_PROTOCOL_SWITCHED_ETHERNET:
                {
                    SwitchedEthernetFinalize(node, interfaceIndex);
                    break;
                }
#endif // ENTERPRISE_LIB
#ifdef MODE5_LIB
                case MAC_PROTOCOL_MODE5: { ; } break;
#endif
#ifdef SATELLITE_LIB
                case MAC_PROTOCOL_SATTSM:
                case MAC_PROTOCOL_ANE:
                {
                    MacAneFinalize(node, interfaceIndex);

                    break;
                }
                case MAC_PROTOCOL_SATELLITE_BENTPIPE:
                {
                    MacSatelliteBentpipeFinalize(node, interfaceIndex);

                    break;
                }
#endif // SATELLITE_LIB

#ifdef CELLULAR_LIB
                case MAC_PROTOCOL_GSM:
                {
                    MacGsmFinalize(node, interfaceIndex);
                    break;
                }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
                case MAC_PROTOCOL_CELLULAR:
                {
                    MacCellularFinalize(node, interfaceIndex);
                    break;
                }
#endif //WIRELESS_LIB

#ifdef ALE_ASAPS_LIB
                case MAC_PROTOCOL_ALE:
                {
                    MacAleFinalize(node, interfaceIndex);
                    break;
                }
#endif // ALE_ASAPS_LIB
#ifdef ADDON_LINK16
                case MAC_PROTOCOL_SPAWAR_LINK16:
                {
                    MacSpawar_Link16Finalize(node, interfaceIndex);
                    break;
                }
#endif // ADDON_LINK16

#ifdef ADVANCED_WIRELESS_LIB
                case MAC_PROTOCOL_DOT16:
                {
                    MacDot16Finalize(node, interfaceIndex);
                    break;
                }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
                case MAC_PROTOCOL_LTE:
                {
                    LteLayer2Finalize(node, interfaceIndex);
                    break;
                }
#endif // LTE_LIB

#ifdef SENSOR_NETWORKS_LIB
                case MAC_PROTOCOL_802_15_4:
                {
                    Mac802_15_4Finalize(node, interfaceIndex);
                    break;
                }
#endif //SENSOR_NETWORKS_LIB


#ifdef CYBER_LIB
                case MAC_PROTOCOL_WORMHOLE:
                {
                    MacWormholeFinalize(node, interfaceIndex);
                    break;
                }
#endif // CYBER_LIB

//InsertPatch FINALIZE_FUNCTION
                default:
                    ERROR_ReportError("Unknown or disabled MAC protocol");
                    break;
            }//switch//

            if (node->macData[interfaceIndex]->randFault)
            {
                MAC_RandFaultFinalize(node, interfaceIndex);
            }
            if (node->macData[interfaceIndex]->bgMainStruct)
            {
                BgTraffic_Finalize(node, interfaceIndex);
            }
        }//if//

#ifdef ADDON_BOEINGFCS
        // make sure we have macData, because some interfaces may not (dualIP)
        if (node->macData[interfaceIndex])
        {
            // Print the new stats ifInOctets and ifOutOctets.
            char buf[MAX_STRING_LENGTH];
            char buf2[MAX_STRING_LENGTH];
            sprintf(buf,
                    "IfInOctets (in bytes) = %d",
                    (UInt32)node->macData[interfaceIndex]->ifInOctets);
            IO_PrintStat(node, "MAC", "", ANY_DEST, interfaceIndex,buf);
            sprintf(buf,
                    "IfOutOctets (in bytes) = %d",
                    (UInt32)node->macData[interfaceIndex]->ifOutOctets);
            IO_PrintStat(node, "MAC", "", ANY_DEST, interfaceIndex,buf);
            sprintf(buf,
                    "IfInErrors = %d",
                    (UInt32)node->macData[interfaceIndex]->ifInErrors);
            IO_PrintStat(node, "MAC", "", ANY_DEST, interfaceIndex,buf);
            sprintf(buf,
                    "IfOutErrors = %d",
                    (UInt32)node->macData[interfaceIndex]->ifOutErrors);
            IO_PrintStat(node, "MAC", "", ANY_DEST, interfaceIndex,buf);
            sprintf(buf,
                    "IfHCInOctets (in bytes) = %d",
                    (UInt64)node->macData[interfaceIndex]->ifHCInOctets);
            IO_PrintStat(node, "MAC", "", ANY_DEST, interfaceIndex,buf);
            sprintf(buf,
                    "IfHCOutOctets (in bytes) = %d",
                    (UInt64)node->macData[interfaceIndex]->ifHCOutOctets);
            IO_PrintStat(node, "MAC", "", ANY_DEST, interfaceIndex,buf);
            if (node->macData[interfaceIndex]->ifOperStatus == 1)
            {
                sprintf(buf, "OperationalState = Up");
                sprintf(buf2, "AdministrativeState = Up");
            }
            else
            {
                sprintf(buf, "OperationalState = Down");
                sprintf(buf2, "AdministrativeState = Down");
            }
            IO_PrintStat(node, "MAC", "", ANY_DEST, interfaceIndex, buf);
            IO_PrintStat(node, "MAC", "", ANY_DEST, interfaceIndex, buf2);
        }
#endif // ADDON_BOEINGFCS

        node->exitInterface();
    }//for//

#ifdef ENTERPRISE_LIB
    // Finalize switch after protocol finalize calls
    if (MAC_IsASwitch(node))
    {
        Switch_Finalize(node);
    }

    // Finalize MPLS statistics, if MPLS is running.
    MplsFinalize(node);
#endif // ENTERPRISE_LIB
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueIsEmpty
// PURPOSE:  Check whether or not the network output queue is empty.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueIsEmpty(Node *node, int interfaceIndex) {

//    ARP Phase
    if (ArpIsEnable(node, interfaceIndex))
    {
        BOOL isEmpty = FALSE;

        if (LlcIsEnabled(node, interfaceIndex))
        {
            isEmpty = ArpQueueIsEmpty(node, interfaceIndex);
            if (!isEmpty)
            {
                return isEmpty;
            }
        }
        else if (node->macData[interfaceIndex]->macProtocol ==
                    MAC_PROTOCOL_802_3)
        {
             isEmpty = ArpQueueIsEmpty(node,interfaceIndex);
             if (!isEmpty)
             {
                return isEmpty;
             }
        }

    }

#ifdef ENTERPRISE_LIB
    if (MAC_IsASwitch(node)) {
        return Switch_QueueIsEmptyForInterface(node, interfaceIndex);
    }

    MplsData *mpls = (MplsData *)node->macData[interfaceIndex]->mplsVar;
    if (mpls)
    {
        // For IP+MPLS, we need to check the IP output queue for both mpls
        // and IP packets. Queue will be empty if both types of packets
        // are not there
        return MplsOutputQueueIsEmpty(node, interfaceIndex);
    }
    else
#endif // ENTERPRISE_LIB
#ifdef ADDON_BOEINGFCS
    if (NetworkCesIncSincgarsActiveOnInterface(node, interfaceIndex))
    {
        return NetworkCesIncSincgarsOutputQueueIsEmpty(node, interfaceIndex);
    }
    else if (NetworkCesIncEplrsActiveOnInterface(node, interfaceIndex)) {
        return NetworkCesIncEplrsIncOutputQueueIsEmpty(node, interfaceIndex);
    }
    else
#endif

    {
        return NetworkIpOutputQueueIsEmpty(node, interfaceIndex);
    }
}


// --------------------------------------------------------------------------
// FUNCTION: MAC_NotificationOfPacketDrop
// PURPOSE:  Notify network layer of packet drop.
// --------------------------------------------------------------------------
void
MAC_NotificationOfPacketDrop(
    Node *node,
    MacHWAddress nextHopAddr,
    int interfaceIndex,
    Message *msg)
{
    LlcHeader* llc = NULL;

    if (LlcIsEnabled(node, (int)DEFAULT_INTERFACE))
    {
        if (msg->headerProtocols[msg->numberOfHeaders - 1] == TRACE_LLC)
        {
            llc = (LlcHeader*) MESSAGE_ReturnPacket(msg);
            LlcRemoveHeader(node, msg);
        }
    }

#ifdef CYBER_LIB
    if (node->macData[interfaceIndex]->isWormhole)
    {
        MESSAGE_Free(node, msg);
        return;
    }
#endif // CYBER_LIB

#ifdef ADDON_BOEINGFCS
    if (NetworkCesIncSincgarsActiveOnInterface(node, interfaceIndex))
    {
        MESSAGE_Free(node, msg);
        return;
    }
#endif



#ifdef ENTERPRISE_LIB
        MplsData *mpls = (MplsData *)node->macData[interfaceIndex]->mplsVar;
#endif // ENTERPRISE_LIB
       if (llc && llc->etherType == PROTOCOL_TYPE_ARP)
       {
            NodeAddress nextHopAddress = MacHWAddressToIpv4Address(node,
                                                    interfaceIndex,
                                                    &nextHopAddr);
            ARPNotificationOfPacketDrop(node,
                                        msg,
                                        nextHopAddress,
                                        interfaceIndex);
       }
#ifdef ENTERPRISE_LIB
        else if (mpls && llc && llc->etherType == PROTOCOL_TYPE_MPLS)
        {
            NodeAddress nextHopAddress = MacHWAddressToIpv4Address(node,
                                                    interfaceIndex,
                                                    &nextHopAddr);
            MplsNotificationOfPacketDrop(node, msg, nextHopAddress,
                                                             interfaceIndex);
        }
#endif // ENTERPRISE_LIB
        else
        {
            IpHeaderType *ipHeader =
                (IpHeaderType *)MESSAGE_ReturnPacket(msg);
            if (IpHeaderGetVersion(ipHeader->ip_v_hl_tos_len) == IPVERSION4)
            {
                NodeAddress nextHopAddress = MacHWAddressToIpv4Address(node,
                                                    interfaceIndex,
                                                    &nextHopAddr);
                node->exitInterface();
                NetworkIpNotificationOfPacketDrop(node,
                                              msg,
                                              nextHopAddress,
                                              interfaceIndex);
                node->enterInterface(interfaceIndex);
            }
            else if (IpHeaderGetVersion(ipHeader->ip_v_hl_tos_len) ==
                IPV6_VERSION)
            {
                node->exitInterface();
                Ipv6NotificationOfPacketDrop(
                    node,
                    msg,
                    nextHopAddr,
                    interfaceIndex);
                node->enterInterface(interfaceIndex);
            }
            else
            {
                MESSAGE_Free(node, msg);
                return;
            }

        }
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_NotificationOfPacketDrop
// PURPOSE:  Notify network layer of packet drop.
// --------------------------------------------------------------------------
void
MAC_NotificationOfPacketDrop(
    Node *node,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    Message *msg)
{

    MacHWAddress lastHopHWAddr;
    MAC_FourByteMacAddressToVariableHWAddress(node, interfaceIndex,
                                             &lastHopHWAddr, nextHopAddress);
    MAC_NotificationOfPacketDrop(node, lastHopHWAddr, interfaceIndex, msg);

}

void
MAC_NotificationOfPacketDrop(
    Node *node,
    Mac802Address nextHopAddress,
    int interfaceIndex,
    Message *msg)
{

    MacHWAddress lastHopHWAddr;
    Convert802AddressToVariableHWAddress(node, &lastHopHWAddr,
                                                            &nextHopAddress);
    MAC_NotificationOfPacketDrop(node, lastHopHWAddr, interfaceIndex, msg);

}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueTopPacketForAPriority
// PURPOSE:  Look at the packet at the front of the specified priority
//           output queue.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    MacHWAddress *destHWAddr)
{
    BOOL aPacket = FALSE;
    NodeAddress nextHopAddress = 0;

    if (ArpIsEnable(node, interfaceIndex))
    {
        aPacket = ArpQueueTopPacket(node, interfaceIndex, msg,
                                    &nextHopAddress, destHWAddr);
        if (aPacket)
        {
            // Here we add the packet to the Network database.
            // Gather as much information we can for the database.
            // Handle IPv4 packets.  IPv6 is not currently supported.
            if (node->networkData.networkProtocol != IPV6_ONLY)
            {
                NetworkIpAddPacketSentToMacDataPoints(
                    node,
                    *msg,
                    interfaceIndex);
            }

            return aPacket;
        }
    }
#ifdef ENTERPRISE_LIB
    MplsData *mpls = (MplsData *)node->macData[interfaceIndex]->mplsVar;
    int posInQueue = 0;
    if (mpls)
    {
        while (!aPacket)
        {
            if (MAC_OutputQueuePeekByIndex(
                    node,
                    interfaceIndex,
                    posInQueue,
                    msg,
                    destHWAddr,
                    &priority))
            {
                LlcHeader* llc;
                llc = (LlcHeader*) MESSAGE_ReturnPacket((*msg));
                if (llc->etherType == PROTOCOL_TYPE_MPLS)
                {
                     aPacket = MplsOutputQueueTopPacketForAPriority(
                                   node,
                                   interfaceIndex,
                                   priority,
                                   msg,
                                   &nextHopAddress,
                                   destHWAddr,
                                   posInQueue);
                }
                else if (llc->etherType == PROTOCOL_TYPE_IP)
                {
                    aPacket = NetworkIpOutputQueueTopPacketForAPriority(
                                 node,
                                 interfaceIndex,
                                 priority,
                                 msg,
                                 &nextHopAddress,
                                 destHWAddr,
                                 posInQueue);
                }
                posInQueue++;
            }
            else
            {
                // There is no packet to be dequeued, so break
                break;
            }
        } // end of while not dequeued
    }
    else
#endif // ENTERPRISE_LIB
    {
        aPacket =
            NetworkIpOutputQueueTopPacketForAPriority(
                node, interfaceIndex, priority, msg,
                &nextHopAddress, destHWAddr);
    }

    if (aPacket)
    {
        // Handle IPv4 packets.  IPv6 is not currently supported.
        if (node->networkData.networkProtocol != IPV6_ONLY)
        {
            NetworkIpAddPacketSentToMacDataPoints(
                node,
                *msg,
                interfaceIndex);
        }
    }

    return aPacket;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueTopPacketForAPriority
// PURPOSE:  Look at the packet at the front of the specified priority
//           output queue.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    NodeAddress *nextHopAddress)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueTopPacketForAPriority(
                                node,interfaceIndex,priority,
                                msg,&destHWAddr);
    if (dequeued)
    {
        *nextHopAddress = MAC_VariableHWAddressToFourByteMacAddress(
                                                          node, &destHWAddr);
    }
    return dequeued;
}
// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueTopPacketForAPriority
// PURPOSE:  Look at the packet at the front of the specified priority
//           output queue.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    Mac802Address *destMacAddr)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueTopPacketForAPriority(node,
                                                         interfaceIndex,
                                                         priority,msg,
                                                          &destHWAddr);
    if (dequeued)
    {
        ConvertVariableHWAddressTo802Address(node, &destHWAddr ,destMacAddr);
    }

    return dequeued;
}

//--------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacketForAPriority
// PURPOSE:  Remove the packet at the front of the specified priority
//           output queue.
//--------------------------------------------------------------------------
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    MacHWAddress *destHWAddr,
    int *networkType)
{
    BOOL dequeued = FALSE;
    NodeAddress nextHopAddress = 0;

    if (ArpIsEnable(node, interfaceIndex))
    {
        dequeued = DequeueArpPacket(node, interfaceIndex, msg,
                                   &nextHopAddress, destHWAddr);
        if (dequeued)
        {
            // Handle IPv4 packets.  IPv6 is not currently supported.
            if (node->networkData.networkProtocol != IPV6_ONLY)
            {
                NetworkIpAddPacketSentToMacDataPoints(
                    node,
                    *msg,
                    interfaceIndex);
            }

            return dequeued;
        }
    }


#ifdef ENTERPRISE_LIB
    MplsData *mpls = (MplsData *)node->macData[interfaceIndex]->mplsVar;
    int posInQueue = 0;
    if (mpls)
    {
        while (!dequeued)
        {
            if (MAC_OutputQueuePeekByIndex(
                    node,
                    interfaceIndex,
                    posInQueue,
                    msg,
                    destHWAddr,
                    &priority))
            {
                LlcHeader* llc;
                llc = (LlcHeader*) MESSAGE_ReturnPacket((*msg));
                if (llc->etherType == PROTOCOL_TYPE_MPLS)
                {
        dequeued =
            MplsOutputQueueDequeuePacketForAPriority(
                            node, interfaceIndex, priority, msg,
                            &nextHopAddress,destHWAddr, posInQueue);
                }
                else if (llc->etherType == PROTOCOL_TYPE_IP)
                {
                   dequeued =
                    NetworkIpOutputQueueDequeuePacketForAPriority(
                        node, interfaceIndex, priority, msg, &nextHopAddress,
                        destHWAddr, networkType, posInQueue);
                }
                posInQueue++;
            }
            else
            {
                // There is no packet to be dequeued, so break
                break;
            }
        } // end of while not dequeued
    }
    else
#endif // ENTERPRISE_LIB
#ifdef ADDON_BOEINGFCS
    if (NetworkCesIncSincgarsActiveOnInterface(node, interfaceIndex))
    {
        dequeued =
            NetworkCesIncSincgarsOutputQueueDequeueFMU(
                node, interfaceIndex, msg, &nextHopAddress);

        MAC_FourByteMacAddressToVariableHWAddress(node,interfaceIndex,
                                                  destHWAddr,nextHopAddress);
    }
    else if (NetworkCesIncEplrsActiveOnInterface(node, interfaceIndex)) { //j.h
        dequeued =
            NetworkCesIncEplrsIncOutputQueueDequeuePacketForAPriority(
                node, interfaceIndex, priority, msg, &nextHopAddress);

        MAC_FourByteMacAddressToVariableHWAddress(node,interfaceIndex,
                                                  destHWAddr,nextHopAddress);
    }
    else
#endif
    {
        dequeued =
            NetworkIpOutputQueueDequeuePacketForAPriority(
                                   node, interfaceIndex, priority, msg,
                                   &nextHopAddress, destHWAddr, networkType);
    }
    if (dequeued)
    {
        // Handle IPv4 packets.  IPv6 is not currently supported.
        if (node->networkData.networkProtocol != IPV6_ONLY)
        {
            NetworkIpAddPacketSentToMacDataPoints(
                node,
                *msg,
                interfaceIndex);
        }
    }

    return dequeued;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacketForAPriority
// PURPOSE:  Remove the packet at the front of the specified priority
//           output queue.
//--------------------------------------------------------------------------
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueDequeuePacketForAPriority(node,
                                               interfaceIndex,
                                               priority,
                                               msg,
                                               &destHWAddr,
                                               networkType);
    if (dequeued)
    {
        *nextHopAddress = MAC_VariableHWAddressToFourByteMacAddress(
                                                          node, &destHWAddr);
    }
    return dequeued;
}

//--------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacketForAPriority
// PURPOSE:  Remove the packet at the front of the specified priority
//           output queue.
//--------------------------------------------------------------------------
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message** msg,
    Mac802Address* destMacAddr,
    int* networkType)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueDequeuePacketForAPriority(node,
                                                            interfaceIndex,
                                                            priority,msg,
                                                            &destHWAddr,
                                                            networkType);
    if (dequeued)
    {
        ConvertVariableHWAddressTo802Address(node, &destHWAddr, destMacAddr);
    }
    return dequeued;

}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacketForAPriority
// PURPOSE:  Remove the packet at the front of the specified priority
//           output queue and also check the packet is for ARP or not.
// RETURN: BOOL
// --------------------------------------------------------------------------

BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType* priority,
    Message** msg,
    NodeAddress *nextHopAddress,
    MacHWAddress* destMacAddr,
    int* networkType,
    int* packetType)
{
    BOOL dequeued = FALSE;
    unsigned short hwLength;
    MacGetHardwareLength(node, interfaceIndex, &hwLength);
    destMacAddr->byte =
        (unsigned char*) MEM_malloc(sizeof(unsigned char)*hwLength);
    destMacAddr->hwLength = hwLength;

    if (ArpIsEnable(node, interfaceIndex))
    {
        if (ArpQueueTopPacket(node, interfaceIndex, msg,
                            nextHopAddress, destMacAddr))
        {
            dequeued = DequeueArpPacket(node, interfaceIndex, msg,
                                        nextHopAddress, destMacAddr);
            if (dequeued)
            {
                *packetType = 1;

                // Handle IPv4 packets.  IPv6 is not currently supported.
                if (node->networkData.networkProtocol != IPV6_ONLY)
                {
                    NetworkIpAddPacketSentToMacDataPoints(
                        node,
                        *msg,
                        interfaceIndex);
                }

                return dequeued;
            }
        }
    }
     //Check whether the is message for dequeue.
     //If there is no message for dequeue, the function returned
     // FALSE value

    if (!MAC_OutputQueueTopPacket(
        node,
        interfaceIndex,
        msg,
        nextHopAddress,
        networkType,
        priority))
    {
        return FALSE;
    }


#ifdef ENTERPRISE_LIB
    MplsData *mpls = (MplsData *)node->macData[interfaceIndex]->mplsVar;
    int posInQueue = 0;
    if (mpls)
    {
        while (!dequeued)
        {
            if (MAC_OutputQueuePeekByIndex(
                    node,
                    interfaceIndex,
                    posInQueue,
                    msg,
                    destMacAddr,
                    priority))
            {
                LlcHeader* llc;
                llc = (LlcHeader*) MESSAGE_ReturnPacket((*msg));
                if (llc->etherType == PROTOCOL_TYPE_MPLS)
                {
        dequeued =
            MplsOutputQueueDequeuePacketForAPriority(
                            node, interfaceIndex, (*priority), msg,
                            nextHopAddress,destMacAddr, posInQueue);


                }
                else if (llc->etherType == PROTOCOL_TYPE_IP)
                {
                   dequeued =
                    NetworkIpOutputQueueDequeuePacketForAPriority(
                        node, interfaceIndex, *priority, msg, nextHopAddress,
                        destMacAddr, networkType, posInQueue);
                }
                posInQueue++;
            }
            else
            {
                // There is no packet to be dequeued, so break
                break;
            }
        } // end of while not dequeued
    }
    else
#endif // ENTERPRISE_LIB
#ifdef ADDON_BOEINGFCS
    if (NetworkCesIncSincgarsActiveOnInterface(node, interfaceIndex))
    {
        dequeued =
            NetworkCesIncSincgarsOutputQueueDequeueFMU(
                node, interfaceIndex, msg, nextHopAddress);
    }
    else
#endif
    {
        dequeued =
            NetworkIpOutputQueueDequeuePacketForAPriority(
                node,
                interfaceIndex,
                *priority,
                msg,
                nextHopAddress,
                destMacAddr,
                networkType);
    }
    if (dequeued)
    {
        // Handle IPv4 packets.  IPv6 is not currently supported.
        if (node->networkData.networkProtocol != IPV6_ONLY)
        {
            NetworkIpAddPacketSentToMacDataPoints(
                node,
                *msg,
                interfaceIndex);
        }
    }

    return dequeued;
}

BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType* priority,
    Message** msg,
    NodeAddress *nextHopAddress,
    Mac802Address* destMacAddr,
    int* networkType,
    int* packetType)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueDequeuePacketForAPriority(
                                            node,
                                            interfaceIndex,
                                            priority,
                                            msg,
                                            nextHopAddress,
                                            &destHWAddr,
                                            networkType,
                                            packetType);
    if (dequeued)
    {
        ConvertVariableHWAddressTo802Address(node, &destHWAddr, destMacAddr);
    }
    return dequeued;

}

// ------------------------------------------------------------------------
// FUNCTION: MAC_SneakPeekAtMacPacket
// PURPOSE:  Allow the network layer to catch a glimpse of the packet
//           before MAC does anything with it and the ARP packet.
// RETURN: void
// ------------------------------------------------------------------------
void
MAC_SneakPeekAtMacPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    MacHWAddress& prevHop,
    MacHWAddress& destAddr,
    int messageType)
{
       Mac802Address destMacAddr;
       ConvertVariableHWAddressTo802Address(node, &destAddr, &destMacAddr);

       int mac_security = 0;

#ifdef CYBER_LIB
       if (msg->headerProtocols[msg->numberOfHeaders-1] == TRACE_MACDOT11_WEP)
       {
           mac_security = TRACE_MACDOT11_WEP;
           MESSAGE_RemoveHeader(node, msg,
               WEP_FRAME_HDR_SIZE, TRACE_MACDOT11_WEP);
       }
       else if (msg->headerProtocols[msg->numberOfHeaders-1] == TRACE_MACDOT11_CCMP)
       {
           mac_security = TRACE_MACDOT11_CCMP;
           MESSAGE_RemoveHeader(node, msg,
               CCMP_FRAME_HDR_SIZE, TRACE_MACDOT11_CCMP);
       }
#endif // CYBER_LIB

       if (LlcIsEnabled(node, interfaceIndex))
       {
            LlcHeader* llc;
            llc = (LlcHeader*) MESSAGE_ReturnPacket(msg);
            LlcRemoveHeader(node, (Message*) msg);


#ifdef ENTERPRISE_LIB
            MplsData *mpls = (MplsData *)
                                      node->macData[interfaceIndex]->mplsVar;
            MacSwitch* sw = Switch_GetSwitchData(node);
#endif // ENTERPRISE_LIB

        // Don't process the packet if the interface is faulty or
        // if mpls is enabled in the interface or
        // if STP and other switch related protocol frames

            if (!MAC_InterfaceIsEnabled(node, interfaceIndex)
#ifdef ENTERPRISE_LIB
                || mpls
                || (sw && Switch_IsStpGroupAddr(destMacAddr,
                                           sw->Switch_stp_address))
#endif // ENTERPRISE_LIB
                )
            {
                return;
            }

            if (llc->etherType == PROTOCOL_TYPE_ARP)
            {
                ArpSneakPacket(node, (Message*) msg, interfaceIndex);
                MESSAGE_AddHeader(node, msg, LLC_HEADER_SIZE, TRACE_LLC);
            }
            else if (llc->etherType == PROTOCOL_TYPE_IP)
            {
                NetworkIpSneakPeekAtMacPacket(node,
                                              msg,
                                              interfaceIndex,
                                              prevHop);
                MESSAGE_AddHeader(node, msg, LLC_HEADER_SIZE, TRACE_LLC);
            }
        }
        else if (messageType == PROTOCOL_TYPE_ARP)
        {
             ArpReceivePacket(node, (Message*) msg, interfaceIndex);
        }
        else
        {
             NetworkIpSneakPeekAtMacPacket(node,
                                           msg,
                                           interfaceIndex,
                                           prevHop);
        }

#ifdef CYBER_LIB
        if (mac_security == TRACE_MACDOT11_WEP)
        {
            MESSAGE_AddHeader(node, msg,
                WEP_FRAME_HDR_SIZE, TRACE_MACDOT11_WEP);
        }
        else if (mac_security == TRACE_MACDOT11_CCMP)
        {
            MESSAGE_AddHeader(node, msg,
                CCMP_FRAME_HDR_SIZE, TRACE_MACDOT11_CCMP);
        }
#endif // CYBER_LIB
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_SneakPeekAtMacPacket
// PURPOSE:  Allow the network layer to catch a glimpse of the packet
//           before MAC does anything with it and the ARP packet.
// RETURN: void
// --------------------------------------------------------------------------
void
MAC_SneakPeekAtMacPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    Mac802Address prevHop,
    Mac802Address destAddr,
    int messageType)
{
    MacHWAddress prevHWaddr;
    MacHWAddress destHWaddr;
    Convert802AddressToVariableHWAddress(node, &prevHWaddr, &prevHop);
    Convert802AddressToVariableHWAddress(node, &destHWaddr, &destAddr);
    MAC_SneakPeekAtMacPacket(node, interfaceIndex, msg, prevHWaddr,
                             destHWaddr, messageType);
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_SneakPeekAtMacPacket
// PURPOSE:  Allow the network layer to catch a glimpse of the packet
//           before MAC does anything with it and the ARP packet.
// RETURN: void
// --------------------------------------------------------------------------
void
MAC_SneakPeekAtMacPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    NodeAddress prevHop,
    NodeAddress destAddr,
    int messageType)
{
    MacHWAddress prevHWaddr;
    MacHWAddress destHWaddr;
    MAC_FourByteMacAddressToVariableHWAddress(node, interfaceIndex,
                                               &prevHWaddr, prevHop);
    MAC_FourByteMacAddressToVariableHWAddress(node, interfaceIndex,
                                              &destHWaddr, destAddr);
    MAC_SneakPeekAtMacPacket(node, interfaceIndex ,msg, prevHWaddr,
                             destHWaddr, messageType);
}
// --------------------------------------------------------------------------
// FUNCTION: MAC_MacLayerAcknowledgement
// PURPOSE:  Allow the network layer to catch a glimpse of the packet
//           before MAC does anything with it.
// --------------------------------------------------------------------------

void MAC_MacLayerAcknowledgement(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    NodeAddress nextHop)
{

    NetworkIpReceiveMacAck(node, interfaceIndex, msg, nextHop);
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_MacLayerAcknowledgement
// PURPOSE:  Allow the network layer to catch a glimpse of the packet
//           before MAC does anything with it.
// --------------------------------------------------------------------------
void MAC_MacLayerAcknowledgement(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    MacHWAddress& nextHop)
{
    NodeAddress nextHop_IpAddr = MacHWAddressToIpv4Address(node,
                                                    interfaceIndex,
                                                    &nextHop);
    NetworkIpReceiveMacAck(node, interfaceIndex, msg, nextHop_IpAddr);
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_MacLayerAcknowledgement
// PURPOSE:  Allow the network layer to catch a glimpse of the packet
//           before MAC does anything with it.
// --------------------------------------------------------------------------
void MAC_MacLayerAcknowledgement(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    Mac802Address& nextHop)
{
    MacHWAddress nextHopHWAddr;
    Convert802AddressToVariableHWAddress(node,&nextHopHWAddr, &nextHop);
    NodeAddress nextHop_IpAddr = MacHWAddressToIpv4Address(node,
                                                    interfaceIndex,
                                                    &nextHopHWAddr);
    NetworkIpReceiveMacAck(node, interfaceIndex, msg, nextHop_IpAddr);
}



// --------------------------------------------------------------------------
// FUNCTION: MAC_HandOffSuccessfullyReceivedPacket
// PURPOSE:  Pass packet up to the network layer.
// --------------------------------------------------------------------------
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    MacHWAddress* macAddr)
{
    if (!MAC_InterfaceIsEnabled(node, interfaceIndex))
    {
        // DB drop events is currently not handled here as in case of
        // disabled interface, the packet will not reach here for supported
        // MAC models. Also, we dont have access to MAC header at this
        // point to extract the information.

        MESSAGE_Free(node, msg);
        return;
    }

#ifdef ADDON_DB
    if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_DOT11
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_3
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_LINK)
    {
        // This is a centralized routine for send to upper mac events.
        // The if statement of protocols is just ones currently using the
        // centralized routine. When other MAC models are converted (there
        // aggregate and summary stats are converted to stats api), they
        // should  clean up uses of MacEvents sentToupper if a call
        // HandOffSuccessfullyReceivedPacket is made directly below it.
        // The model will then need to be added to the if statement.

        HandleMacDBEvents(node,
                          msg,
                          node->macData[interfaceIndex]->phyNumber,
                          interfaceIndex,
                          MAC_SendToUpper,
                          node->macData[interfaceIndex]->macProtocol);
    }
#endif /* ADDON_DB */

    if (LlcIsEnabled(node, interfaceIndex))
    {
        LlcHeader* llc;
        llc = (LlcHeader*) MESSAGE_ReturnPacket(msg);
        LlcRemoveHeader(node, msg);

        if (llc->etherType == PROTOCOL_TYPE_ARP)
        {
            ArpReceivePacket(node, msg, interfaceIndex);
            return;
        }
#ifdef ENTERPRISE_LIB
       else if (llc->etherType == PROTOCOL_TYPE_MPLS)
       {
            MplsData *mpls =
                (MplsData *)node->macData[interfaceIndex]->mplsVar;
            if (mpls)
            {

              NodeAddress lastHopAddress =
                        MacHWAddressToIpv4Address(node,
                                                  interfaceIndex,
                                                  macAddr);
              MplsReceivePacketFromMacLayer(node,
                                            interfaceIndex,
                                            msg,
                                            lastHopAddress);
            }
        }
#endif
        else if (llc->etherType == PROTOCOL_TYPE_IP)
    {
            node->exitInterface();
            NETWORK_ReceivePacketFromMacLayer(node,
                                              msg,
                                              macAddr,
                                              interfaceIndex);
            node->enterInterface(interfaceIndex);
    }
    else
    {
            ERROR_Assert(FALSE, "Unknown LLC Ether Type in LLC Header");
        }
    }
   else    //if llc not enabled by default IP Packet,
   {
       node->exitInterface();
#ifdef ADDON_BOEINGFCS
       if (NetworkCesIncIsEnable(node, interfaceIndex))
       {
           NetworkCesIncReceivePacket(
               node,
               interfaceIndex,
               msg);
           node->enterInterface(interfaceIndex);
           return;
       }
       else if (node->macData[interfaceIndex]->macProtocol ==
           MAC_PROTOCOL_LINK_EPLRS)
       {
           NetworkCesIncReceivePacketForEPLRS(
               node,
               interfaceIndex,
               msg);
           node->enterInterface(interfaceIndex);
           return;
       }
#endif // ADDON_BOEINGFCS
                NETWORK_ReceivePacketFromMacLayer(node,
                                                  msg,
                                                  macAddr,
                                                  interfaceIndex);
                node->enterInterface(interfaceIndex);
    }
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_HandOffSuccessfullyReceivedPacket
// PURPOSE:  Pass packet up to the network layer.
// --------------------------------------------------------------------------
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    NodeAddress lastHopAddress)
{
    MacHWAddress lastHopHWAddr;
    MAC_FourByteMacAddressToVariableHWAddress(node,
                                              interfaceIndex,
                                              &lastHopHWAddr,
                                              lastHopAddress);


    MAC_HandOffSuccessfullyReceivedPacket(node,
                                          interfaceIndex,
                                          msg,
                                          &lastHopHWAddr);

}

void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    Mac802Address* lastHopAddr)
{
    MacHWAddress lastHopHWAddr;
    Convert802AddressToVariableHWAddress(node,&lastHopHWAddr,lastHopAddr);
    MAC_HandOffSuccessfullyReceivedPacket(node,
                                         interfaceIndex,
                                         msg,
                                         &lastHopHWAddr);

}

// --------------------------------------------------------------------------
// FUNCTION: MAC_HandOffSuccessfullyReceivedPacket
// PURPOSE: Pass packet up to the network layer and also check whether the
//          packet is for ARP.Used only in 802.3 protocol to handle ARP when
//            LLCis not enabled
// RETURN: void
// --------------------------------------------------------------------------
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    MacHWAddress* macAddr,
    int messageType)
{
    if (!MAC_InterfaceIsEnabled(node, interfaceIndex))
    {
        // DB drop events is currently not handled here as in case of
        // disabled interface, the packet will not reach here for supported
        // MAC models. Also, we dont have access to MAC header at this
        // point to extract the information.

        MESSAGE_Free(node, msg);
        return;
    }

#ifdef ADDON_DB
    if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_DOT11
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_3
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_LINK)
    {
        // This is a centralized routine for send to upper mac events.
        // The if statement of protocols is just ones currently using the
        // centralized routine. When other MAC models are converted (there
        // aggregate and summary stats are converted to stats api), they
        // should  clean up uses of MacEvents sentToupper if a call
        // HandOffSuccessfullyReceivedPacket is made directly below it.
        // The model will then need to be added to the if statement.

        HandleMacDBEvents(node,
                          msg,
                          node->macData[interfaceIndex]->phyNumber,
                          interfaceIndex,
                          MAC_SendToUpper,
                          node->macData[interfaceIndex]->macProtocol);
    }
#endif /* ADDON_DB */

    if (LlcIsEnabled(node, interfaceIndex))
    {
        LlcHeader* llc;
        llc = (LlcHeader*) MESSAGE_ReturnPacket(msg);
        LlcRemoveHeader(node, msg);

        if (llc->etherType == PROTOCOL_TYPE_ARP)
    {
        ArpReceivePacket(node, msg, interfaceIndex);
        return;
    }

#ifdef ENTERPRISE_LIB
        else if (llc->etherType == PROTOCOL_TYPE_MPLS)
        {
            MplsData *mpls =
                (MplsData *)node->macData[interfaceIndex]->mplsVar;
    if (mpls)
    {
              NodeAddress lastHopAddress =
                        MacHWAddressToIpv4Address(node,interfaceIndex,macAddr);
              MplsReceivePacketFromMacLayer(node,
            interfaceIndex,
            msg,
            lastHopAddress);
    }
        }
#endif
        else if (llc->etherType == PROTOCOL_TYPE_IP)
        {
            node->exitInterface();
            NETWORK_ReceivePacketFromMacLayer(node,
                                              msg,
                                              macAddr,
                                              interfaceIndex);
            node->enterInterface(interfaceIndex);
        }
    else
    {
            ERROR_Assert(FALSE, "Unknown LLC Ether Type in LLC Header");
        }
    }
    else
    {
        if (ArpIsEnable(node, interfaceIndex))
        {
            ArpReceivePacket(node, msg, interfaceIndex);
            return;
        }

        node->exitInterface();
        NETWORK_ReceivePacketFromMacLayer(node,
                                          msg,
                                          macAddr,
                                          interfaceIndex);
        node->enterInterface(interfaceIndex);
    }
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_HandOffSuccessfullyReceivedPacket
// PURPOSE: Pass packet up to the network layer and also check whether the
//          packet is for ARP.Used only in 802.3 protocol to handle ARP when
//            LLCis not enabled
// RETURN: void
// --------------------------------------------------------------------------
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    Mac802Address* lastHopAddr,
    int messageType)
{
    MacHWAddress lastHopHWAddr;
    Convert802AddressToVariableHWAddress(node, &lastHopHWAddr, lastHopAddr);
    MAC_HandOffSuccessfullyReceivedPacket(node, interfaceIndex, msg,
                                          &lastHopHWAddr, messageType);
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueTopPacket
// PURPOSE:  Look at the packet at the front of the output queue.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    MacHWAddress* destHWAddr,
    int* networkType,
    TosType *priority)
{
    BOOL aPacket = FALSE;

    *networkType = NETWORK_PROTOCOL_IP;
     NodeAddress nextHopAddress;
    int posInQueue = 0;

     if (ArpIsEnable(node, interfaceIndex))
     {
        aPacket = ArpQueueTopPacket(node, interfaceIndex, msg,
                                        &nextHopAddress, destHWAddr);
        if (aPacket)
        {
            return aPacket;
        }
      }

#ifdef ENTERPRISE_LIB
    MplsData *mpls = (MplsData *)node->macData[interfaceIndex]->mplsVar;

    if (mpls)
    {
        while (!aPacket)
        {
            if (MAC_OutputQueuePeekByIndex(
                    node,
                    interfaceIndex,
                    posInQueue,
                    msg,
                    destHWAddr,
                    priority))
            {
                LlcHeader* llc;
                llc = (LlcHeader*) MESSAGE_ReturnPacket((*msg));
                if (llc->etherType == PROTOCOL_TYPE_MPLS)
                {
                     aPacket = MplsOutputQueueTopPacket(
                                    node,
                                    interfaceIndex,
                                    msg,
                                    &nextHopAddress,
                                    destHWAddr,
                                    priority,
                                    posInQueue);
                }
                else if (llc->etherType == PROTOCOL_TYPE_IP)
                {
                    aPacket = NetworkIpOutputQueueTopPacket(
                                 node,
                                 interfaceIndex,
                                 msg,
                                 &nextHopAddress,
                                 destHWAddr,
                                 networkType,
                                 priority,
                                 posInQueue);
                }
                posInQueue++;
            }
            else
            {
                // There is no packet to be dequeued, so break
                break;
            }
        } // end of while not dequeued
    }
    else
#endif // ENTERPRISE_LIB
#ifdef ADDON_BOEINGFCS
    if (NetworkCesIncSincgarsActiveOnInterface(node, interfaceIndex)) {
        aPacket =
            NetworkCesIncSincgarsOutputQueueTopFMU(
                node, interfaceIndex, msg, &nextHopAddress);

        MAC_FourByteMacAddressToVariableHWAddress(node,interfaceIndex,
                                                  destHWAddr,nextHopAddress);
    }
    else if (NetworkCesIncEplrsActiveOnInterface(node, interfaceIndex) &&
        (NetworkCesIncIsEnable(node, interfaceIndex) &&
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_EPLRS)
        == FALSE)
    { //j.h
        aPacket =
            NetworkCesIncEplrsIncOutputQueueTopPacket(
            node, interfaceIndex, msg, &nextHopAddress, priority);
        if (aPacket)
        {
            MAC_FourByteMacAddressToVariableHWAddress(node,interfaceIndex,
                destHWAddr,nextHopAddress);
        }
    }
    else if (aPacket == FALSE)
#endif
    {
        aPacket =
            NetworkIpOutputQueueTopPacket(
                node,
                interfaceIndex,
                msg,
                &nextHopAddress,
                destHWAddr,
                networkType,
                priority);
    }
    return aPacket;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueTopPacket
// PURPOSE:  Look at the packet at the front of the output queue.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int* networkType,
    TosType *priority)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueTopPacket(node, interfaceIndex, msg,
                                &destHWAddr, networkType, priority);
    if (dequeued)
    {
        *nextHopAddress = MAC_VariableHWAddressToFourByteMacAddress(
                                                          node, &destHWAddr);
    }
    return dequeued;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueTopPacket
// PURPOSE:  Look at the packet at the front of the output queue.
// It is a
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    Mac802Address* destMacAddr,
    int* networkType,
    TosType *priority)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueTopPacket(node, interfaceIndex, msg,
                             &destHWAddr, networkType, priority);
    if (dequeued)
    {
        ConvertVariableHWAddressTo802Address(node, &destHWAddr, destMacAddr);
    }
    return dequeued;

}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacket
// PURPOSE:  Remove the packet at the front of the output queue.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType,
    TosType *priority)
{

    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueDequeuePacket(node, interfaceIndex, msg,
                                        &destHWAddr, networkType, priority);
    if (dequeued)
    {
        *nextHopAddress = MAC_VariableHWAddressToFourByteMacAddress(
                                                          node, &destHWAddr);
    }
    return dequeued;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacket
// PURPOSE:  Remove the packet at the front of the output queue.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    MacHWAddress *destHWAddr,
    int *networkType,
    TosType *priority)
{
    BOOL dequeued = FALSE;
    NodeAddress nextHopAddress = 0;

     if (ArpIsEnable(node, interfaceIndex))
    {
        dequeued = DequeueArpPacket(node, interfaceIndex, msg,
                                    &nextHopAddress, destHWAddr);
        if (dequeued)
        {
            // Handle IPv4 packets.  IPv6 is not currently supported.
            if (node->networkData.networkProtocol != IPV6_ONLY)
            {
                NetworkIpAddPacketSentToMacDataPoints(
                    node,
                    *msg,
                    interfaceIndex);
            }

            return dequeued;
        }
    }
    int posInQueue = 0;
#ifdef ENTERPRISE_LIB
    MplsData *mpls = (MplsData *) node->macData[interfaceIndex]->mplsVar;

    if (mpls)
    {
        while (!dequeued)
        {
            if (MAC_OutputQueuePeekByIndex(
                    node,
                    interfaceIndex,
                    posInQueue,
                    msg,
                    destHWAddr,
                    priority))
            {
                LlcHeader* llc;
                llc = (LlcHeader*) MESSAGE_ReturnPacket((*msg));
                if (llc->etherType == PROTOCOL_TYPE_MPLS)
                {
                    dequeued = MplsOutputQueueDequeuePacket(
                                   node,
                                   interfaceIndex,
                                   msg,
                                   &nextHopAddress,
                                   destHWAddr,
                                   priority,
                                   posInQueue);
                }
                else if (llc->etherType == PROTOCOL_TYPE_IP)
                {
                    dequeued = NetworkIpOutputQueueDequeuePacket(
                                   node,
                                   interfaceIndex,
                                   msg,
                                   &nextHopAddress,
                                   destHWAddr,
                                   networkType,
                                   priority,
                                   posInQueue);
                }
                posInQueue++;
            }
            else
            {
                // There is no packet to be dequeued, so break
                break;
            }
        } // end of while not dequeued
    }
    else
#endif // ENTERPRISE_LIB
#ifdef ADDON_BOEINGFCS
    if (NetworkCesIncSincgarsActiveOnInterface(node, interfaceIndex))
    {
        dequeued =
            NetworkCesIncSincgarsOutputQueueDequeueFMU(
                node, interfaceIndex, msg, &nextHopAddress);

        NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
        NetworkCesIncData* inc =
            ip->interfaceInfo[interfaceIndex]->networkCesIncData;

        if (inc != NULL && inc->enable)
        {
            *destHWAddr = GetBroadCastAddress(node, interfaceIndex);
        }
        else
        {
            MAC_FourByteMacAddressToVariableHWAddress(node,interfaceIndex,
                destHWAddr,nextHopAddress);
        }
    }
    else
#endif
    {

        dequeued =
            NetworkIpOutputQueueDequeuePacket(
                node,
                interfaceIndex,
                msg,
                &nextHopAddress,
                destHWAddr,
                networkType,
                priority);
    }

    if (dequeued)
    {
        // Handle IPv4 packets.  IPv6 is not currently supported.
        if (node->networkData.networkProtocol != IPV6_ONLY)
        {
            NetworkIpAddPacketSentToMacDataPoints(
                node,
                *msg,
                interfaceIndex);
        }
    }

    return dequeued;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacket
// PURPOSE:  Remove the packet at the front of the output queue.
//           Overloaded Function
// --------------------------------------------------------------------------
BOOL MAC_OutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    Mac802Address* destMacAddr,
    int *networkType,
    TosType *priority)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueDequeuePacket(node, interfaceIndex, msg,
                                &destHWAddr, networkType, priority);
    if (dequeued)
    {
        ConvertVariableHWAddressTo802Address(node, &destHWAddr, destMacAddr);
    }
    return dequeued;
}

// --------------------------------------------------------------------------
// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacket
// LAYER        :: MAC
// PURPOSE      :: To remove packet(s) from front of output queue;
//                 process packets with options
//                 for example, pakcing multiple packets with
//                 same next hop address together
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + msg            : Message** : Pointer to Message
// + nextHopAddress : NodeAddress* : Pointer to Node address
// + networkType    : int* : network type
// + priority       : TosType * : Pointer to queuing priority type
// + dequeueOption  : MacOutputQueueDequeueOption :
//                                   option to dequeue output queue
// + dequeueCriteria: MacOutputQueueDequeueCriteria :
//                                    criteria used when to retrive packets
// + numFreeByte    : int * : number of bytes can be packed in 1 transmission
// + numPacketPacked : int* : number of packets packed
// + tracePrt      : TraceProtocolType : Trace protocol type
// + eachWithMacHeader : BOOL : Each msg has its own MAC header?
//                              if not, PHY/MAC header size should be
//                              accounted before call this functions;
//                              otherwise,only PHY header considered.
// + maxHeaderSize : int : max mac header size
// + returnPackedMsg : BOOL : return Packed msg or a list of msgs
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/

BOOL MAC_OutputQueueDequeuePacket(
                Node* node,
                int interfaceIndex,
                Message **msg,
                NodeAddress* nextHopAddress,
                int* networkType,
                int* priority,
                MacOutputQueueDequeueOption dequeueOption,
                MacOutputQueueDequeueCriteria dequeueCriteria,
                int* numFreeByte,
                int* numPackedPackets,
                TraceProtocolType tracePrt,
                BOOL eachWithMacHeader,
                int maxHeaderSize,
                BOOL returnPackedMsg)
{
        // retrive the first packet recode the first nexthopAddress
        // while loop index to retrive more packets meet the
        // if more than one packets retrived, packed it

        // Assume the next hop for the first packet "master packet" in queue;
        // it could be other criteria to determine
        // which packet can be the master packet, e.g.,
        // longest waiting time, best channel quality, highest qos weight,...

        // if pack option is used, FIFO may be violated. It also affect the
        // performace due to exhaustive search

        NodeAddress nextHopMasterMsg;
        int priorityMasterMsg;
        BOOL dequeued = FALSE;
        Message* firstMsg = NULL;
        Message* lastMsg = NULL;

        // get the master packet
        dequeued = MAC_OutputQueueDequeuePacket(
                        node,
                        interfaceIndex,
                        msg,
                        &nextHopMasterMsg,
                        networkType,
                        &priorityMasterMsg);
        if (dequeued)
        {
            firstMsg = *msg;
        }
        else
        {
            *msg = NULL;

            return dequeued;
        }

        // set last Msg
        lastMsg = firstMsg; // at this point, lastmsg must be not NULL

        // update the available freeByte
        *numFreeByte = *numFreeByte - MESSAGE_ReturnPacketSize(firstMsg);

        if (eachWithMacHeader)
        {
            *numFreeByte = *numFreeByte - maxHeaderSize;
        }
        // update the stats for pakced packets
        // increase accrodingly unicast/brodcast/multicast
        (*numPackedPackets) ++;

        // in case the first packet is a big packet coming from upper,
        // let MAC layer itself handle it, either drop it or fragment it
        // no further process for option
        // Only pack option and same next hop as criteria are considered now
        if (dequeueOption == MAC_DEQUEUE_OPTION_PACK &&
            (dequeueCriteria == MAC_DEQUEUE_PACK_SAME_NEXT_HOP ||
            dequeueCriteria == MAC_DEQUEUE_PACK_ANY_PACKET) &&
            *numFreeByte > 0)
        {
            BOOL keepSearch = TRUE;
            int posInQueue = 0;

            Message* tempMsg = NULL;
            NodeAddress nextHopTempMsg;
            int priorityTempMsg;
            MacHWAddress nextHopMacAddr;
            BOOL dequeueSuc = FALSE;

            while (keepSearch)
            {
                // if last dequeue is successful and has space left
                // continun to look for proper packets meet the requirement

                dequeueSuc = FALSE;

                // First, peek the packet in the queue, get the msg
                dequeued = NetworkIpOutputQueueTopPacket(
                               node,
                               interfaceIndex,
                               &tempMsg,
                               &nextHopTempMsg,
                               &nextHopMacAddr,
                               networkType,
                               &priorityTempMsg,
                               posInQueue);
                if (dequeued &&
                    ((dequeueCriteria == MAC_DEQUEUE_PACK_SAME_NEXT_HOP &&
                     nextHopTempMsg == nextHopMasterMsg) ||
                    dequeueCriteria == MAC_DEQUEUE_PACK_ANY_PACKET))
                {
                    if ((*numFreeByte <  MESSAGE_ReturnPacketSize(tempMsg) &&
                         !eachWithMacHeader) ||
                        (*numFreeByte < (maxHeaderSize +
                            MESSAGE_ReturnPacketSize(tempMsg)) &&
                         eachWithMacHeader))
                    {
                        // to avoid out of order packets, break from here
                        // fragmentation of a IP packet will be doen in future
                        break;
                    }

                    // retrive the packet from the queue and
                    // put in the msg list
                    dequeued = NetworkIpOutputQueueDequeuePacket(
                                   node,
                                   interfaceIndex,
                                   &tempMsg,
                                   &nextHopTempMsg,
                                   &nextHopMacAddr,
                                   networkType,
                                   &priorityTempMsg,
                                   posInQueue);
                    ERROR_Assert(dequeued,
                                "A dequeue error happened after peeking");

                    // update the free byte
                    *numFreeByte = *numFreeByte -
                                        MESSAGE_ReturnPacketSize(tempMsg);

                    if (eachWithMacHeader)
                    {
                        *numFreeByte = *numFreeByte - maxHeaderSize;
                    }

                    tempMsg->next = NULL; // reset in case  not set to NULL
                    lastMsg->next = tempMsg;
                    lastMsg = tempMsg;

                    // update the stats
                    // increase accordingly for unicast/brodcast/multicast
                    (*numPackedPackets) ++;
                    dequeueSuc = TRUE;
                    // optimization
                    // if freebyte is smaller than a threshold then give up
                    // this free byte instead of searching
                    // if (*numFreeByte <= 10 )
                    // {
                    //     keepSearch = FALSE;
                    // }
                }
                else if (!dequeued)
                {
                    // no more packet in queue
                    keepSearch = FALSE;
                }
                else
                {
                    // if the messages has been ordered
                    // by size or any other criteria,
                    // keepSearch may be set to FALSE here
                }

                // move to next pakcet
                if (dequeueSuc == FALSE)
                {
                    posInQueue ++;
                }
            }
        }
        else
        {
            // do nothing right now
        }

        // pack messags if needed
        if (*numPackedPackets > 1 &&
             dequeueOption == MAC_DEQUEUE_OPTION_PACK &&
             returnPackedMsg)
        {
            firstMsg = MESSAGE_PackMessage(node, firstMsg, tracePrt, NULL);
            // need to give the right trace value for other protocols
        }

        *msg = firstMsg;
        *nextHopAddress = nextHopMasterMsg;
        *priority = priorityMasterMsg; // the priority has no meaning if
                                       // packets of diff. prio. are packed

        // Handle IPv4 packets.  IPv6 is not currently supported.
        if (node->networkData.networkProtocol != IPV6_ONLY)
        {
            NetworkIpAddPacketSentToMacDataPoints(
                node,
                *msg,
                interfaceIndex);
        }

        return TRUE; // at this point, at least one packet is dequeued
}

// --------------------------------------------------------------------------
// /**
// FUNCTION     :: MAC_OutputQueueDequeuePacketForAPriority
// LAYER        :: MAC
// PURPOSE      :: To remove packet(s) from front of output queue;
//                 process packets with options
//                 for example, pakcing multiple packets with
//                 same next hop address together
// PARAMETERS   ::
// + node           : Node* : Pointer to a network node
// + interfaceIndex : int : interfaceIndex
// + priority       : TosType * : Pointer to queuing priority type
// + msg            : Message** : Pointer to Message
// + nextHopAddress : NodeAddress* : Pointer to Node address
// + networkType    : int* : network type
// + dequeueOption  : MacOutputQueueDequeueOption :
//                                      option to dequeue output queue
// + dequeueCriteria: MacOutputQueueDequeueCriteria :
//                                     criteria used when to retrive packets
// + numFreeByte    : int * : number of bytes can be packed in 1 transmission
// + numPacketPacked : int* : number of packets packed
// + tracePrt : TraceProtocolType : Trace Protocol Type
// + eachWithMacHeader : BOOL : Each msg has its own MAC header?
//                              if not, PHY/MAC header size should be
//                              accounted before call this functions;
//                              otherwise,only PHY header considered.
// + maxHeaderSize : int : max mac header size
// + returnPackedMsg : BOOL : return Packed msg or a list of msgs
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
// **/

BOOL MAC_OutputQueueDequeuePacketForAPriority(
                Node* node,
                int interfaceIndex,
                int priority,
                Message **msg,
                NodeAddress* nextHopAddress,
                int* networkType,
                MacOutputQueueDequeueOption dequeueOption,
                MacOutputQueueDequeueCriteria dequeueCriteria,
                int* numFreeByte,
                int* numPackedPackets,
                TraceProtocolType tracePrt,
                BOOL eachWithMacHeader,
                int maxHeaderSize,
                BOOL returnPackedMsg)
{
        // retrive the first packet recode the first nexthopAddress
        // while loop index to retrive more packets meet the
        // if more than one packets retrived, packed it

        // Assume the next hop for the first packet "master packet" in queue;
        // it could be other criteria to determine
        // which packet can be the master packet, e.g.,
        // longest waiting time, best channel quality, highest qos weight,....

        // if pack option is used, FIFO may be violated. It also affect the
        // performace due to exhaustive search

        NodeAddress nextHopMasterMsg;
        int priorityMasterMsg;
        BOOL dequeued = FALSE;
        Message* firstMsg = NULL;
        Message* lastMsg = NULL;

        // get the master packet
        priorityMasterMsg = priority;
        dequeued = MAC_OutputQueueDequeuePacketForAPriority(
                        node,
                        interfaceIndex,
                        priorityMasterMsg,
                        msg,
                        &nextHopMasterMsg,
                        networkType);
        if (dequeued)
        {
            firstMsg = *msg;
        }
        else
        {
            *msg = NULL;

            return dequeued;
        }

        // set last Msg
        lastMsg = firstMsg; // at this point, lastmsg must be not NULL

        // update the available freeByte
        *numFreeByte = *numFreeByte - MESSAGE_ReturnPacketSize(firstMsg);

        if (eachWithMacHeader)
        {
            *numFreeByte = *numFreeByte - maxHeaderSize;
        }
        // update the stats for pakced packets
        // increase accrodingly unicast/brodcast/multicast
        (*numPackedPackets) ++;


        // in case the first packet is a big packet coming from upper,
        // let MAC layer itself handle it, either drop it or fragment it
        // no further process for option
        // Only pack option and same next hop as criteria are considered now
        if (dequeueOption == MAC_DEQUEUE_OPTION_PACK &&
           (dequeueCriteria == MAC_DEQUEUE_PACK_SAME_NEXT_HOP ||
            dequeueCriteria == MAC_DEQUEUE_PACK_ANY_PACKET) &&
            *numFreeByte > 0)
        {
            BOOL keepSearch = TRUE;
            int posInQueue = 0;

            Message* tempMsg = NULL;
            NodeAddress nextHopTempMsg;
            int priorityTempMsg;
            MacHWAddress nextHopMacAddr;
            BOOL dequeueSuc = FALSE;

            while (keepSearch)
            {
                // if last dequeue is successful and has space left
                // continun to look for proper packets meet the requirement

                dequeueSuc = FALSE;

                // First, peek the packet in the queue, get the msg
                priorityTempMsg = priorityMasterMsg;
                dequeued = NetworkIpOutputQueueTopPacketForAPriority(
                               node,
                               interfaceIndex,
                               priorityTempMsg,
                               &tempMsg,
                               &nextHopTempMsg,
                               &nextHopMacAddr,
                               posInQueue);
                if (dequeued &&
                    ((dequeueCriteria == MAC_DEQUEUE_PACK_SAME_NEXT_HOP &&
                    nextHopTempMsg == nextHopMasterMsg) ||
                    dequeueCriteria == MAC_DEQUEUE_PACK_ANY_PACKET))
                {
                    if ((*numFreeByte <  MESSAGE_ReturnPacketSize(tempMsg) &&
                          !eachWithMacHeader) ||
                          (*numFreeByte < (maxHeaderSize +
                          MESSAGE_ReturnPacketSize(tempMsg)) &&
                          eachWithMacHeader))
                    {
                        // to avoid out of order packets, break from here
                        // fragmentation of a IP packet will be doen in future
                        break;
                    }
                    // retrive the packet from the queue & put in the msg list
                    dequeued = NetworkIpOutputQueueDequeuePacketForAPriority(
                                    node,
                                    interfaceIndex,
                                    priorityTempMsg,
                                    &tempMsg,
                                    &nextHopTempMsg,
                                    &nextHopMacAddr,
                                    networkType,
                                    posInQueue);
                    ERROR_Assert(dequeued,
                                "A dequeue error happened after peeking");

                    // update the free byte
                    *numFreeByte = *numFreeByte -
                                        MESSAGE_ReturnPacketSize(tempMsg);

                    if (eachWithMacHeader)
                    {
                        *numFreeByte = *numFreeByte - maxHeaderSize;
                    }

                    tempMsg->next = NULL; // reset in case not set to NULL
                    lastMsg->next = tempMsg;
                    lastMsg = tempMsg;

                    // update the stats
                    // increase accordingly for unicast/brodcast/multicast
                    (*numPackedPackets) ++;
                    dequeueSuc = TRUE;
                    // optimization
                    // if freebyte is smaller than a threshold then
                    // give up this free byte instead of searching
                    // if (*numFreeByte <= 10 )
                    // {
                    //     keepSearch = FALSE;
                    // }
                }
                else if (!dequeued)
                {
                    // no more packet in queue
                    keepSearch = FALSE;
                }
                else
                {
                    // if the messages has been ordered
                    // by size or any other criteria,
                    // keepSearch may be set to FALSE here
                }

                // move to next pakcet
                if (dequeueSuc == FALSE)
                {
                    posInQueue ++;
                }
            }
        }
        else
        {
            // do nothing right now
        }

        // pack messags if needed
        if (*numPackedPackets > 1 &&
             dequeueOption == MAC_DEQUEUE_OPTION_PACK &&
             returnPackedMsg)
        {
            firstMsg = MESSAGE_PackMessage(node, firstMsg, tracePrt, NULL);
            // need to give the right trace value for other protocols
        }

        *msg = firstMsg;
        *nextHopAddress = nextHopMasterMsg;

        return TRUE; // at this point, at least one packet is dequeued
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_IsMyUnicastFrame
// PURPOSE:  Check if a packet (or frame) belongs to this node.
//           Should be used only for 4 byte mac address
// --------------------------------------------------------------------------
BOOL MAC_IsMyUnicastFrame(Node *node, NodeAddress destAddr)
{
    MacHWAddress destHWAddr;
    MAC_FourByteMacAddressToVariableHWAddress(node, CPU_INTERFACE,
                                             &destHWAddr, destAddr);

    return  MAC_IsMyAddress(node, &destHWAddr);
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_SetInterfaceStatusHandlerFunction
// PURPOSE:  Set the interface handler function to be called when interface
//           faults occur.
// --------------------------------------------------------------------------
void
MAC_SetInterfaceStatusHandlerFunction(Node *node,
                                      int interfaceIndex,
                                      MacReportInterfaceStatus statusHandler)
{
    if (TunnelIsVirtualInterface(node, interfaceIndex))
    {
        TunnelSetInterfaceStatusHandlerFunction(
                            node, interfaceIndex, statusHandler);
        return;
    }
    assert(node->macData[interfaceIndex]->interfaceStatusHandlerList != NULL);

    ListInsert(node,
               node->macData[interfaceIndex]->interfaceStatusHandlerList,
               node->getNodeTime(),
               (void*) statusHandler);
}


// --------------------------------------------------------------------------
// FUNCTION: MAC_IsWiredNetwork
// PURPOSE:  Determines if an interface is a wired interface.
// --------------------------------------------------------------------------
BOOL
MAC_IsWiredNetwork(Node *node, int interfaceIndex)
{
    // Abstract satellite model is considered a wired network
    // for the time being.

    if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_LINK ||
//      node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_MPLS ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_3 ||
        node->macData[interfaceIndex]->macProtocol ==
                                              MAC_PROTOCOL_SWITCHED_ETHERNET)
    {
        return TRUE;
    }

    return FALSE;
}


// --------------------------------------------------------------------------
// FUNCTION: MAC_IsPointToPointNetwork
// PURPOSE:  Determines if an interface is a wired broadcast interface.
// --------------------------------------------------------------------------
BOOL
MAC_IsPointToPointNetwork(Node *node, int interfaceIndex)
{
    if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_LINK)
    {
        LinkData* link = (LinkData*)node->macData[interfaceIndex]->macVar;
        if (link->isPointToPointLink)
        {
            return TRUE;
        }
    }
    else if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_3)
    {
        MacData802_3* mac802_3 = (MacData802_3*)node->macData[interfaceIndex]->macVar;
        if (mac802_3->link->isPointToPointLink)
        {
            return TRUE;
        }
    }

    return FALSE;
}


// --------------------------------------------------------------------------
// FUNCTION: MAC_IsPointToMultiPointNetwork
// PURPOSE:  Determines if an interface is a point to multi-point network.
// --------------------------------------------------------------------------
BOOL
MAC_IsPointToMultiPointNetwork(Node *node, int interfaceIndex)
{
    if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_DOT16)
    {
        return TRUE;
    }

    return FALSE;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_IsWiredBroadcastNetwork
// PURPOSE:  Determines if an interface is a wired broadcast interface.
// --------------------------------------------------------------------------
BOOL
MAC_IsWiredBroadcastNetwork(Node *node, int interfaceIndex)
{
    // Abstract satellite model is considered a wired network
    // for the time being.

    if (node->macData[interfaceIndex]->macProtocol ==
                                            MAC_PROTOCOL_SWITCHED_ETHERNET ||
       (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_3
            && ((MacData802_3* ) (node->macData[interfaceIndex]->macVar))
                                                  ->isFullDuplex == FALSE ))
    {
        return TRUE;
    }

    return FALSE;
}


// --------------------------------------------------------------------------
// FUNCTION: MAC_IsWirelessNetwork
// PURPOSE:  Determines if an interface is a wireless interface.
// --------------------------------------------------------------------------
BOOL
MAC_IsWirelessNetwork(Node *node, int interfaceIndex)
{
    if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_11 ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CSMA ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_FCSC_CSMA ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_MACA ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_FAMA ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_SATCOM ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_GSM ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_SPAWAR_LINK16 ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_TADIL_LINK11 ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_TADIL_LINK16 ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_DOT16 ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_TDMA ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_SATTSM ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_ANE ||
#ifdef MODE5_LIB
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_MODE5 ||
#endif
#ifdef ADDON_BOEINGFCS
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WINTNCW ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WINTHNW ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WINTGBS ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WNW_MDL ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_SRW ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_SRW_PORT ||
#endif // ADDON_BOEINGFCS
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_DOT11 ||
#ifdef CYBER_LIB
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_WORMHOLE ||
#endif // CYBER_LIB
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_SATELLITE_BENTPIPE)
    {
        return TRUE;
    }
    return FALSE;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_IsWirelessAdHocNetwork
// PURPOSE:  Determines if an interface is a wireless interface.
// --------------------------------------------------------------------------
BOOL
MAC_IsWirelessAdHocNetwork(Node *node, int interfaceIndex)
{
    if (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_11 ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CSMA ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_MACA ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_FAMA ||
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_TDMA ||
#ifdef CYBER_LIB
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_WORMHOLE ||
#endif // CYBER_LIB
#ifdef ADDON_BOEINGFCS
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WNW_MDL ||
#endif
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_DOT11)
    {
        return TRUE;
    }

    return FALSE;
}

// --------------------------------------------------------------------------
// FUNCTION: MAC_IsOneHopBroadcastNetwork
// PURPOSE:  Determines if an interface is a wireless interface.
// --------------------------------------------------------------------------
BOOL
MAC_IsOneHopBroadcastNetwork(Node *node, int interfaceIndex)
{
    if (node->macData[interfaceIndex]->macProtocol ==
                                               MAC_PROTOCOL_SWITCHED_ETHERNET
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CSMA
        || node->macData[interfaceIndex]->macProtocol ==
                                               MAC_PROTOCOL_FCSC_CSMA
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_SATCOM
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_11
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_TDMA
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_GSM
        || (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_3
             && ((MacData802_3* ) (node->macData[interfaceIndex]->macVar))
                                                   ->isFullDuplex == FALSE )
        || (node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_LINK
             && ((LinkData*)node->macData[interfaceIndex]->macVar)
                                             ->isPointToPointLink == FALSE)
#ifdef CYBER_LIB
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_WORMHOLE
#endif // CYBER_LIB
#ifdef ADDON_BOEINGFCS
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WNW_MDL
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WINTNCW
        || node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WINTGBS
#endif // ADDON_BOEINGFCS
        || node->macData[interfaceIndex]->macProtocol ==
                                            MAC_PROTOCOL_SATELLITE_BENTPIPE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


// NAME:        MAC_IsASwitch
//
// PURPOSE:     Check if a node is a switch.
//
// PARAMETERS:  This node
//
// RETURN:      TRUE, if node acting as a switch
//              FALSE, otherwise

BOOL MAC_IsASwitch(
    Node* node)
{
    if (node->switchData)
    {
        return TRUE;
    }
    else
    {
        // Switch pointer is NULL. So it is a node.
        return FALSE;
    }
}



void MAC_SetVirtualMacAddress(Node *node,
                              int interfaceIndex,
                              NodeAddress virtualMacAddress)
{
    node->macData[interfaceIndex]->virtualMacAddress = virtualMacAddress;
}


//-----------------------------------------------------------------------
// NAME:        MAC_RandFaultInit
// PURPOSE:     Initialization the Random Fault structure from input file
// PARAMETERS:  faultyNode - in which node this structure is initialized.
//              interfaceIndex - in which interface.
//              currentLine - pointer to the input string.
// RETURN:      None
//------------------------------------------------------------------------
void
MAC_RandFaultInit(Node* faultyNode,
                  int interfaceIndex,
                  const char* currentLine)
{
    int nToken;
    clocktype interval;
    Message* selfTimerMsg;
    char temStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char* tokenStr;
    RandFault* randFaultPtr;
    RandomSeed seed;

    RandomDistribution<clocktype> timeDistribution;
    RandomDistribution<Int32>     integerDistribution;
    RANDOM_SetSeed(seed, faultyNode->globalSeed,
                         faultyNode->nodeId,
                         RANDOM_FAULT,
                         interfaceIndex);

    if (faultyNode->macData[interfaceIndex]->randFault != NULL)
    {
        sprintf(temStr,
            "RAND_LINK_FAULT: Into the NODE %u at %d Interface:"
            " Random fault declaration more than one time.\n ",
            faultyNode->nodeId, interfaceIndex);
        ERROR_ReportError(temStr);
    }

    // memory allocate of the random fault structure
    randFaultPtr = (RandFault*) MEM_malloc(sizeof(RandFault));
    memset(randFaultPtr, 0, sizeof(RandFault));

    strcpy(temStr, currentLine);
    tokenStr = temStr;

    faultyNode->macData[interfaceIndex]->randFault = randFaultPtr;

    // Skip interface and special parameter
    tokenStr = IO_SkipToken(
                   tokenStr, RAND_TOKENSEP, RAND_FAULT_GEN_SKIP);
    sscanf(tokenStr, "%s", buf);

    // Read no of repetition of random fault.
    if (strcmp(buf, "REPS") == 0)
    {
        tokenStr = IO_SkipToken(tokenStr, RAND_TOKENSEP, 1);

        nToken = integerDistribution.setDistribution(tokenStr,
                 "RAND_FAULT", RANDOM_INT);
        randFaultPtr->repetition = integerDistribution.getRandomNumber(seed);

        tokenStr = IO_SkipToken(tokenStr, RAND_TOKENSEP, nToken);
        sscanf(tokenStr, "%s", buf);
    }

    if (!randFaultPtr->repetition)
    {
        randFaultPtr->repetition = -1;
    }

    // Read start time of random fault.
    if (strcmp(buf, "START") == 0)
    {
        tokenStr = IO_SkipToken(tokenStr, RAND_TOKENSEP, 1);

        nToken = timeDistribution.setDistribution(tokenStr, "RAND_FAULT",
                RANDOM_CLOCKTYPE);
        randFaultPtr->startTm = timeDistribution.getRandomNumber(seed);

        tokenStr = IO_SkipToken(tokenStr, RAND_TOKENSEP, nToken);
        sscanf(tokenStr, "%s", buf);
    }

    // Read  mean time between failure of random fault.
    if (strcmp(buf, "MTBF") == 0)
    {
        tokenStr = IO_SkipToken(tokenStr, RAND_TOKENSEP, 1);

        randFaultPtr->randomUpTm.setSeed(faultyNode->globalSeed,
                                         faultyNode->nodeId,
                                         RANDOM_FAULT,
                                         interfaceIndex + 1);
        nToken = randFaultPtr->randomUpTm.setDistribution(tokenStr,
                "RAND_FAULT", RANDOM_CLOCKTYPE);

        tokenStr = IO_SkipToken(tokenStr, RAND_TOKENSEP, nToken);
        sscanf(tokenStr, "%s", buf);
    }
    else
    {
        ERROR_ReportError("MTBF must be into the input line");
    }

    // Read  down time of random fault.
    if (strcmp(buf, "DOWN") == 0)
    {
        tokenStr = IO_SkipToken(tokenStr, RAND_TOKENSEP, 1);

        randFaultPtr->randomDownTm.setSeed(faultyNode->globalSeed,
                                           faultyNode->nodeId,
                                           RANDOM_FAULT,
                                           interfaceIndex + 2);
        nToken = randFaultPtr->randomDownTm.setDistribution(tokenStr,
                "RAND_FAULT", RANDOM_CLOCKTYPE);

        tokenStr = IO_SkipToken(tokenStr, RAND_TOKENSEP, nToken);
    }
    else
    {
        ERROR_ReportError("DOWN must be into the input line");
    }


    if (RAND_FAULT_DEBUG)
    {
        char time[MAX_STRING_LENGTH];

        printf("RAND_LINK_FAULT: In NODE %u at Interface %u:--\n",
                                faultyNode->nodeId, interfaceIndex);
        printf("\tRepetition:-%d\n",randFaultPtr->repetition);
        TIME_PrintClockInSecond(randFaultPtr->startTm, time, faultyNode);
        printf("\tStart Time:-%s\n", time);
        printf("\tFor UP Time Distribution:-\n");
        //randFaultPtr->randomUpTm.printDistributionValue();
        printf("\tFor DOWN Time Distribution:-\n");
        printf("\t");
        //randFaultPtr->randomDownTm.printDistributionValue();
        printf("\n\n");
    }

    //message allocate for self timer.
    selfTimerMsg = MESSAGE_Alloc(
             faultyNode,
             MAC_LAYER,
             0,
             MSG_MAC_StartFault);

    interval = randFaultPtr->randomUpTm.getRandomNumber();

    interval += randFaultPtr->startTm; // if start time is not ZERO.

    MESSAGE_SetInstanceId(selfTimerMsg, (short) interfaceIndex);

    //this information into the info field of self timer message is require
    //to handle static and random fault by one pair of event message
    MESSAGE_InfoAlloc(faultyNode, selfTimerMsg, sizeof(FaultType));
    *((FaultType*)MESSAGE_ReturnInfo(selfTimerMsg)) = RANDOM_FAULT;

    MESSAGE_Send(faultyNode, selfTimerMsg, interval);
}


//-----------------------------------------------------------------------
// NAME:        MAC_RandFaultFinalize
// PURPOSE:     Print the statistics of Random link fault.
// PARAMETERS:  node - in which node
//              interfaceIndex - in which interface.
// RETURN:      None
//------------------------------------------------------------------------
void
MAC_RandFaultFinalize(Node *node, int interfaceIndex)
{
    char buf[100];
    char time[MAX_STRING_LENGTH];
    clocktype avgFault = 0;
    RandFault* randFaultPtr =
        (RandFault*)node->macData[interfaceIndex]->randFault;

    sprintf(buf, "Total number of faults = %d", randFaultPtr->numLinkfault);
    IO_PrintStat(node, "MAC", "Random Fault", ANY_DEST, interfaceIndex, buf);

    TIME_PrintClockInSecond(randFaultPtr->totDuration, time);
    sprintf(buf, "Total Fault time(s) = %s", time);
    IO_PrintStat(node, "MAC", "Random Fault", ANY_DEST, interfaceIndex, buf);

    avgFault = randFaultPtr->totDuration / randFaultPtr->numLinkfault;
    TIME_PrintClockInSecond(avgFault, time);
    sprintf(buf, "Average fault time(s) = %s", time);
    IO_PrintStat(node, "MAC", "Random Fault", ANY_DEST, interfaceIndex, buf);

    sprintf(buf, "Total frames dropped = %d", randFaultPtr->totFramesDrop);
    IO_PrintStat(node, "MAC", "Random Fault", ANY_DEST, interfaceIndex, buf);
}

//---------------------------------------------------------------------------
// FUNCTION           : MAC_IsMyHWAddress
// PURPOSE            : Checks for own MAC address at a particular interface
//                      and for broadcast address.
// PARAMETERS         :
// +node              : Node* node :: Node pointer
// +interfaceIndex    : int interfaceIndex :: Interface index
// +macAddr           : MacAddress* macAddr :: Mac Address
// RETURN             : Bool
// NOTE               : Overloaded MAC_IsMyHWAddress()
// --------------------------------------------------------------------------

BOOL MAC_IsMyHWAddress(
    Node* node,
    int interfaceIndex,
    MacHWAddress* macAddr)
{
    MacHWAddress* myMacAddr = &node->macData[interfaceIndex]->macHWAddr;

    if (MAC_IsIdenticalHWAddress(myMacAddr, macAddr) ||
            MAC_IsBroadcastHWAddress(macAddr))
    {
        return TRUE;
    }
    return FALSE;
}


//-------------------------------------------------------------------------
// FUNCTION           : MAC_IsMyAddress
// PURPOSE            : Checks for own MAC address at all interfaces of
//                      this node
// PARAMETERS         :
// +node              : Node* node :: Node pointer
// +macAddr           : MacAddress* macAddr :: Mac Address
// RETURN             : Bool
// NOTE               : Overloaded MAC_IsMyHWAddress()
// ------------------------------------------------------------------------

BOOL MAC_IsMyAddress(
    Node* node,
    MacHWAddress* macAddr)
{
    unsigned int i = 0;

    for (; i < (unsigned int)node->numberInterfaces; i++)
    {
        if (TunnelIsVirtualInterface(node, i))
        {
            continue;
        }
        MacHWAddress* myMacAddr = &node->macData[i]->macHWAddr;
        if (MAC_IsIdenticalHWAddress(myMacAddr, macAddr))
        {
            return TRUE;
        }
    }
    return FALSE;
}



//---------------------------------------------------------------------------
// FUNCTION           : MAC_IsMyAddress
// PURPOSE            : Checks for own MAC address at a particular
//                      interface of this node
// PARAMETERS         :
// +node              : Node* node :: Node pointer
// +interfaceIndex    : int interfaceIndex :: Interface index
// +macAddr           : MacAddress* macAddr :: Mac Address
// RETURN             : Bool
// NOTE               : Overloaded MAC_IsMyHWAddress()
// --------------------------------------------------------------------------

BOOL MAC_IsMyAddress(
    Node* node,
    int interfaceIndex,
    MacHWAddress* macAddr)
{

    MacHWAddress* myMacAddr = &node->macData[interfaceIndex] ->macHWAddr;

    if (MAC_IsIdenticalHWAddress(myMacAddr, macAddr))
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: MAC_IsBroadcastHWAddress
// LAYER      :: MAC
// PURPOSE    :: Checks Broadcast MAC address
// PARAMETERS ::
// + macAddr : MacHWAddress* : structure to hardware address
// RETURN     :: BOOL : TRUE or FALSE
// **/
BOOL MAC_IsBroadcastHWAddress(
    MacHWAddress* macAddr)
{
    int i = 0;
    BOOL isBroadCast = FALSE;

    for (i = (macAddr->hwLength - 1); i >= 0; i--)
    {
        if (macAddr->byte[i] != 0xff)
        {
            isBroadCast = FALSE;
            break;
        }
        else
        {
            isBroadCast = TRUE;
        }
    }
    return isBroadCast;
}


//---------------------------------------------------------------------------
// FUNCTION           : MAC_IsBroadcastMac802Address
// PURPOSE            : Checks Broadcast MAC address.
// PARAMETERS         :
// +macAddr           : Mac802Address* macAddr :: Mac Address
// RETURN             : Bool
// --------------------------------------------------------------------------

BOOL MAC_IsBroadcastMac802Address(Mac802Address *macAddr)
{
    int i = 0;

    for (i = (MAC_ADDRESS_LENGTH_IN_BYTE - 1); i >= 0; i--)
    {
        if (macAddr->byte[i] != 0xff)
        {
            break;
        }
    }

    if (i == -1)
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: MAC_IsIdenticalHWAddress
// LAYER      :: MAC
// PURPOSE    :: Compares two MAC addresses
// PARAMETERS ::
// + macAddr1 : MacHWAddress* : Pointer to harware address structure
// + macAddr2 : MacHWAddress* : Pointer to harware address structure
// RETURN     :: BOOL : TRUE or FALSE
// **/
BOOL MAC_IsIdenticalHWAddress(
    MacHWAddress* macAddr1,
    MacHWAddress* macAddr2)
{

    if (macAddr1->hwType != macAddr2->hwType ||
            macAddr1->hwLength != macAddr2->hwLength)
    {
        return FALSE;
    }
    else
    {
        if (memcmp(macAddr1->byte, macAddr2->byte ,macAddr1->hwLength) == 0)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

//---------------------------------------------------------------------------
// FUNCTION           : MAC_IsIdenticalMac802Address
// PURPOSE            : Compares two MAC addresses.
// PARAMETERS         :
// +macAddr1          : MacAddress* macAddr1 :: Mac Address
// +macAddr2          : MacAddress* macAddr2 :: Mac Address
// RETURN             : Bool
// --------------------------------------------------------------------------

BOOL MAC_IsIdenticalMac802Address(Mac802Address *macAddr1,
                                  Mac802Address *macAddr2)
{

    if (memcmp(macAddr1->byte, macAddr2->byte, MAC_ADDRESS_LENGTH_IN_BYTE)
                                                                        == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: MAC_CopyMacHWAddress
// LAYER      :: MAC
// PURPOSE    ::  Copies Hardware address address
// PARAMETERS ::
// + destAddr : MacHWAddress* : structure to destination hardware address
// + srcAddr : MacHWAddress* : structure to source hardware address
// RETURN     :: NULL
// **/

void MAC_CopyMacHWAddress(
            MacHWAddress* destAddr,
            MacHWAddress* srcAddr)
{

    destAddr->hwType = srcAddr->hwType;
    destAddr->hwLength = srcAddr->hwLength;
    (*destAddr).byte = (unsigned char*) MEM_malloc(
                      sizeof(unsigned char)*destAddr->hwLength);

    memcpy(destAddr->byte, srcAddr->byte, destAddr->hwLength);

}



// /**
// FUNCTION   :: MAC_CopyMac802Address
// LAYER      :: MAC
// PURPOSE    ::  Copies Hardware address address
// PARAMETERS ::
// + destAddr : Mac802Address* : structure to destination hardware address
// + srcAddr : Mac802Address* : structure to source hardware address
// RETURN     :: NULL
// **/

void MAC_CopyMac802Address(
            Mac802Address* destAddr,
            Mac802Address* srcAddr)
{

        memcpy(destAddr->byte, srcAddr->byte, MAC_ADDRESS_DEFAULT_LENGTH);

}



// /**
// FUNCTION   :: MAC_GetPacketsPriority
// LAYER      :: MAC
// PURPOSE    :: Returns the priority of the packet
// PARAMETERS ::
// + msg       : Message* :Pointer to the packet
// RETURN     :: TosType : priority
// **/

TosType MAC_GetPacketsPriority(Node* node, Message* msg)
{
    TosType priority = 0;
    IpHeaderType *ipHeader = NULL;
    char *payload = MESSAGE_ReturnPacket(msg);

    if (LlcIsEnabled(node, (int)DEFAULT_INTERFACE))
    {
        LlcHeader* llcHeader = (LlcHeader*) payload;
        if (llcHeader->etherType == PROTOCOL_TYPE_ARP)
        {
            return (IPTOS_PREC_INTERNETCONTROL >> 5);
        }

        payload = payload + sizeof(LlcHeader);

#ifdef ENTERPRISE_LIB
        if (llcHeader->etherType == PROTOCOL_TYPE_MPLS)
        {
            payload = payload + sizeof(Mpls_Shim_LabelStackEntry);
        }
#endif // ENTERPRISE_LIB
    }

    ipHeader = (IpHeaderType*) payload;

    if (IpHeaderGetVersion(ipHeader->ip_v_hl_tos_len) == IPVERSION4)
    {
        // Get the Precedence value from IP header TOS type.
        // Shift ECN and Priority then get the precedence
        priority = (IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len) >> (2 + 3));
    }
    else
    {
        ip6_hdr* ipv6Header = NULL;
        ipv6Header = (ip6_hdr* ) payload;
        priority = (ip6_hdrGetClass(ipv6Header->ipv6HdrVcf)) >> 5;
    }

    return (priority);
}


// --------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueuePeekByIndex
// PURPOSE:  Look at the packet at the index of the output queue.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueuePeekByIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    MacHWAddress *nextHopAddr,
    TosType *priority)
{
    BOOL aPacket = FALSE;

     NodeAddress nextHopAddress = 0;
     if (ArpIsEnable(node, interfaceIndex))
    {
        aPacket = ArpQueueTopPacket(node, interfaceIndex, msg,
                                        &nextHopAddress, nextHopAddr);
        if (aPacket)
        {
            return aPacket;
        }
    }

    aPacket =
        NetworkIpOutputQueuePeekWithIndex(node, interfaceIndex, msgIndex,
                                          msg, &nextHopAddress,
                                          nextHopAddr, priority);

    return aPacket;
}

//--------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueuePeekByIndex
// PURPOSE:  Look at the packet at the index of the output queue.
// --------------------------------------------------------------------------
BOOL MAC_OutputQueuePeekByIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    TosType *priority)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueuePeekByIndex(node,
                                               interfaceIndex,
                                               msgIndex,
                                               msg,
                                               &destHWAddr,
                                               priority);
    if (dequeued)
    {
        *nextHopAddress =
            MAC_VariableHWAddressToFourByteMacAddress(node, &destHWAddr);
    }
    return dequeued;
}

BOOL MAC_OutputQueuePeekByIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    Mac802Address *nextHopMacAddr,
    TosType *priority)
{

    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueuePeekByIndex(node, interfaceIndex,
                                               msgIndex, msg, &destHWAddr,
                                               priority);
    if (dequeued)
    {
        ConvertVariableHWAddressTo802Address(node, &destHWAddr,
                                                            nextHopMacAddr);
    }
    return dequeued;
}

//--------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacketWithIndex
// PURPOSE:  To remove the packet at specified index
//           output queue.
//--------------------------------------------------------------------------
BOOL MAC_OutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    MacHWAddress* nextHopMacAddr,
    int *networkType)
{
    BOOL dequeued = FALSE;
    NodeAddress nextHopAddress = 0;

     if (ArpIsEnable(node, interfaceIndex))
    {
        dequeued = DequeueArpPacket(node, interfaceIndex, msg,
                                    &nextHopAddress, nextHopMacAddr);
        if (dequeued)
        {
            return dequeued;
        }
    }

    dequeued =
        NetworkIpOutputQueueDequeuePacketWithIndex( node, interfaceIndex,
                                                    msgIndex, msg,
                                                    &nextHopAddress,
                                                    nextHopMacAddr,
                                                    networkType);

    return dequeued;
}


//--------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacketWithIndex
// PURPOSE:  To remove the packet at specified index
//           output queue.
//--------------------------------------------------------------------------
BOOL MAC_OutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueDequeuePacketWithIndex(node,
                                                          interfaceIndex,
                                                          msgIndex,
                                                          msg,
                                                          &destHWAddr,
                                                          networkType);
    if (dequeued)
    {
        *nextHopAddress = MAC_VariableHWAddressToFourByteMacAddress(
                                                          node, &destHWAddr);
    }
    return dequeued;
}

//--------------------------------------------------------------------------
// FUNCTION: MAC_OutputQueueDequeuePacketWithIndex
// PURPOSE:  To remove the packet at specified index
//           output queue.
//--------------------------------------------------------------------------
BOOL MAC_OutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    Mac802Address* nextHopMacAddr,
    int *networkType)
{
    MacHWAddress destHWAddr;
    BOOL dequeued = MAC_OutputQueueDequeuePacketWithIndex(node,
                                                          interfaceIndex,
                                                          msgIndex,
                                                          msg,
                                                          &destHWAddr,
                                                          networkType);
    if (dequeued)
    {
        ConvertVariableHWAddressTo802Address(node,
                                            &destHWAddr, nextHopMacAddr);
    }
    return dequeued;
}

//--------------------------------------------------------------------------
// FUNCTION: GetBroadCastAddress
// PURPOSE:  Returns Broadcast Address of an interface
// PARAMETERS
// + node : Node* : Pointer to a node
// + interfaceIndex : int: interface of a node
// RETURN MacHWAddress broadcast mac address of a interface
//--------------------------------------------------------------------------
MacHWAddress GetBroadCastAddress(Node *node,int interfaceIndex)
{
    MacHWAddress broadcastHW;
    broadcastHW.hwLength = node->macData[interfaceIndex]->macHWAddr.hwLength;
    broadcastHW.byte = (unsigned char*)MEM_malloc(broadcastHW.hwLength);
    broadcastHW.hwType = node->macData[interfaceIndex]->macHWAddr.hwType;
    memset(broadcastHW.byte, 0xff, broadcastHW.hwLength);
    return broadcastHW;
}

//--------------------------------------------------------------------------
// FUNCTION: GetMacHWAddress
// PURPOSE:  Returns MacHWAddress of an interface
// PARAMETERS
// + node : Node* : Pointer to a node
// + interfaceIndex : int: interface of a node
// RETURN MacHWAddress mac address of a interface
//--------------------------------------------------------------------------
MacHWAddress GetMacHWAddress(Node* node,int interfaceIndex)
{
    return node->macData[interfaceIndex]->macHWAddr;
}


//--------------------------------------------------------------------------
// FUNCTION: MacGetInterfaceIndexFromMacAddress
// PURPOSE:  Returns interfaceIndex at which Macaddress is configured
// PARAMETERS
// + node : Node* : Pointer to a node
// + macAddr : MacHWAddress: Mac Address of a node
// RETURN int  interfaceIndex of node
//--------------------------------------------------------------------------
int MacGetInterfaceIndexFromMacAddress(Node* node,
                                   MacHWAddress& macAddr)
{
    int i;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        MacHWAddress ownhwAddr = GetMacHWAddress(node, i);

       if (MAC_IsIdenticalHWAddress(&ownhwAddr, &macAddr))
       {
           return i;
       }
    }
    return -1;
}

//--------------------------------------------------------------------------
// FUNCTION: MacGetInterfaceIndexFromMacAddress
// PURPOSE:  Returns interfaceIndex at which Macaddress is configured
// PARAMETERS
// + node : Node* : Pointer to a node
// + macAddr : Mac802Address: Mac Address of a node
// RETURN int  interfaceIndex of node
//--------------------------------------------------------------------------
int MacGetInterfaceIndexFromMacAddress(Node* node,
                          Mac802Address& macAddr)
{

    MacHWAddress linkAddr;

    Convert802AddressToVariableHWAddress(node, &linkAddr, &macAddr);

    return MacGetInterfaceIndexFromMacAddress(node, linkAddr);
}

//--------------------------------------------------------------------------
// FUNCTION: MacGetInterfaceIndexFromMacAddress
// PURPOSE:  Returns interfaceIndex at which Macaddress is configured
// PARAMETERS
// + node : Node* : Pointer to a node
// + macAddr : NodeAddress: Mac Address of a node
// RETURN int  interfaceIndex of node
//--------------------------------------------------------------------------
int MacGetInterfaceIndexFromMacAddress(Node* node,
                          NodeAddress macAddr)
{

    MacHWAddress linkAddr;
    MAC_FourByteMacAddressToVariableHWAddress(node, DEFAULT_INTERFACE,
                                                         &linkAddr, macAddr);
    return MacGetInterfaceIndexFromMacAddress(node, linkAddr);
}


static const std::string ConvertToUpper(const std::string& str)
{
    std::string up;

    up = str;
    for (unsigned int i = 0; i < up.length(); i++)
    {
        up[i] = (char) toupper(up[i]);
    }

    return up;
}

void D_Fault::ExecuteAsString(const std::string& in, std::string& out)
{
    std::string upIn;

    if (in.length() <= 3)
    {
        upIn = ConvertToUpper(in);

        if (upIn == "YES")
        {
            Message* msg;
            MacFaultInfo* macFaultInfo;

            // The protocol is set to 0 -- it is ignored
            msg = MESSAGE_Alloc(m_Node,
                                MAC_LAYER,
                                0,
                                MSG_MAC_StartFault);

            MESSAGE_SetInstanceId(msg, (short) m_InterfaceIndex);

            // This information is required to handle static and
            // random fault by one pair of event message
            MESSAGE_InfoAlloc(
                m_Node,
                msg,
                sizeof(MacFaultInfo));
            macFaultInfo = (MacFaultInfo*) MESSAGE_ReturnInfo(msg);
            macFaultInfo->faultType = EXTERNAL_FAULT;

            MESSAGE_SendAsEarlyAsPossible(m_Node, msg);
        }
        else if (upIn == "NO")
        {
            Message* msg;
            MacFaultInfo* macFaultInfo;

            // The protocol is set to 0 -- it is ignored
            msg = MESSAGE_Alloc(m_Node,
                                MAC_LAYER,
                                0,
                                MSG_MAC_EndFault);

            MESSAGE_SetInstanceId(msg, (short) m_InterfaceIndex);

            // This information is required to handle static and
            // random fault by one pair of event message
            MESSAGE_InfoAlloc(
                m_Node,
                msg,
                sizeof(MacFaultInfo));
            macFaultInfo = (MacFaultInfo*) MESSAGE_ReturnInfo(msg);
            macFaultInfo->faultType = EXTERNAL_FAULT;

            MESSAGE_SendAsEarlyAsPossible(m_Node, msg);

        }
    }

    out = "";
}

// /**
// FUNCTION   :: MAC_Reset
// LAYER      :: MAC
// PURPOSE    :: Reset the Mac protocols use by the node
// PARAMETERS ::
// + node      : Node* :Pointer to the node
// + InterfaceIndex : int :  interface index
// **/
void
MAC_Reset(Node* node, int interfaceIndex)
{
#ifdef ADDON_NGCNMS
    struct_mac_reset_function_list* functionList;
    int i;

    //Function Ptrs
    for (i = 0; i < node->numberInterfaces; i++)
    {
        functionList = node->macData[i]->resetFunctionList->first;
        if (interfaceIndex == i || interfaceIndex == -1)
        {
            while (functionList != NULL)
            {
                (functionList->funcPtr)(node, i);
                functionList = functionList->next;
            }
        }
    }
#endif
}

// /**
// FUNCTION   :: MAC_AddResetFunctionList
// LAYER      :: MAC
// PURPOSE    :: Add which protocols in the Mac layer to be reset to a
//              fuction list pointer.
// PARAMETERS ::
// + node      : Node* :Pointer to the node
// + InterfaceIndex : int :  interface index
// + param: void* : pointer to the protocols reset function
// **/
void MAC_AddResetFunctionList(Node* node, int interfaceIndex, void *param)
{
#ifdef ADDON_NGCNMS
    struct_mac_reset_function_list* functionList;

    functionList = new struct_mac_reset_function_list;
    functionList->funcPtr = (MacSetFunctionPtr)param;
    functionList->next = NULL;

    if (node->macData[interfaceIndex]->resetFunctionList->first == NULL)
    {
        node->macData[interfaceIndex]->resetFunctionList->last = functionList;
        node->macData[interfaceIndex]->resetFunctionList->first =
            node->macData[interfaceIndex]->resetFunctionList->last;
    }
    else
    {
        node->macData[interfaceIndex]->resetFunctionList->last->next = functionList;
        node->macData[interfaceIndex]->resetFunctionList->last = functionList;
    }
#endif
}

//--------------------------------------------------------------------------
// FUNCTION: MacSkipLLCandMPLSHeader
// PURPOSE:  skips MPLS and LLC header if present.
// PARAMETERS
// + node : Node* : Pointer to a node
// + payload : char*: pointer to the data packet.
// RETURN Pointer to the data after skipping headers.
//--------------------------------------------------------------------------
char* MacSkipLLCandMPLSHeader(Node* node, char* payload)
{
    char* retPtr = payload;
    if (LlcIsEnabled(node,(int)DEFAULT_INTERFACE))
    {
        LlcHeader* llc;
        llc = (LlcHeader*)retPtr;

        retPtr += sizeof(LlcHeader);
#ifdef ENTERPRISE_LIB
        if (llc->etherType == PROTOCOL_TYPE_MPLS)
        {
            Mpls_Shim_LabelStackEntry shim_label;
            BOOL bottomOfStack;

            // keep removing the MPLS Shim header 1-by-1 untill
            // bottom of stack is reached
            while (1)
            {
                bottomOfStack = FALSE;
                // get the next shim label
                memcpy(&shim_label,retPtr ,
                    sizeof(Mpls_Shim_LabelStackEntry));
                bottomOfStack = MplsShimBottomOfStack(&shim_label);
                retPtr += sizeof(Mpls_Shim_LabelStackEntry);
                if (bottomOfStack)
                {
                    break;
                } //end of if bottomOfStack
            }// end of while (1)
        } // end of if llc->etherType
#endif // ENTERPRISE_LIB
    }// end of if LLC Enabled

    return retPtr;
}

//--------------------------------------------------------------------------
//FUNCTION   :: Mac802AddressToStr
//LAYER      :: MAC
//PURPOSE    :: Convert addr to hex format e.g. [aa-bb-cc-dd-ee-ff]
//PARAMETERS ::
//+ addrStr   : char*         : Converted address string
//                              (This must be pre-allocated memory)
//+ addr      : Mac802Address*   : Pointer to MAC address
//RETURN     :: void
//--------------------------------------------------------------------------
void Mac802AddressToStr(char* addrStr,
                        const Mac802Address* macAddr)
{
    if (addrStr)
    {
        sprintf(addrStr, "[%02X-%02X-%02X-%02X-%02X-%02X]",
            macAddr->byte[0],
            macAddr->byte[1],
            macAddr->byte[2],
            macAddr->byte[3],
            macAddr->byte[4],
            macAddr->byte[5]);
    }
}

//--------------------------------------------------------------------------
//FUNCTION   :: IsSwitchCtrlPacket
//LAYER      :: MAC
//PURPOSE    :: Returns true if msg is control packet for a switch
//PARAMETERS :: Node* node
//                   Pointer to the node
//              Mac802Address 
//                  Destination MAC address
//RETURN     :: BOOL
//--------------------------------------------------------------------------
BOOL IsSwitchCtrlPacket(Node* node,
                        Mac802Address destMacAddr)
{
    MacSwitch* sw = Switch_GetSwitchData(node);
    if ((Switch_IsStpGroupAddr(destMacAddr,
                               sw->Switch_stp_address))
        || (Switch_IsGarpAware(sw)
             && Switch_IsGarpGroupAddr(destMacAddr,sw)))
    {
        return TRUE;
    }
    return FALSE;
}

#ifdef ADDON_DB
//--------------------------------------------------------------------------
// FUNCTION: HandleMacDBEvents
// PURPOSE:  to process the Stats DB event inputa.
// PARAMETERS
// + node : Node* : Pointer to a node
// + msg : Message*: pointer to the frame.
// RETURN Pointer to the data after skipping headers.
//--------------------------------------------------------------------------
void HandleMacDBEvents(
    Node *node,
    Message* msg,
    int phyIndex,
    int interfaceIndex,
    MacDBEventType eventType,
    MAC_PROTOCOL protocol,
    BOOL failureTypeSpecifed,
    std::string failureType,
    BOOL processMacHeader)
{
    // Insert on to the table
    // Check first if the table exists.
    StatsDb* db = node->partitionData->statsDb;

    if (!db || !db->statsEventsTable->createMacEventsTable)
    {
        // Table does not exist.
        return;
    }
    if (!(protocol == MAC_PROTOCOL_DOT11
            || protocol == MAC_PROTOCOL_802_3
            || protocol == MAC_PROTOCOL_LINK
#ifdef LTE_LIB
            || protocol == MAC_PROTOCOL_LTE
#endif // LTE_LIB
#ifdef ADDON_BOEINGFCS
            || protocol == MAC_PROTOCOL_CES_WINTNCW
            || protocol == MAC_PROTOCOL_CES_WINTHNW
            || protocol == MAC_PROTOCOL_CES_WINTGBS
            || protocol == MAC_PROTOCOL_CES_SRW
            || protocol == MAC_PROTOCOL_CES_SRW_PORT
            || protocol == MAC_PROTOCOL_CES_SINCGARS
            || protocol == MAC_PROTOCOL_CES_EPLRS
            || protocol == MAC_PROTOCOL_CES_WNW_MDL
#endif
    ))
    {
        //ERROR_ReportWarning("StatsDB is not supported.");
        return;
    }

    std::string eventStr;
    switch (eventType)
    {
    case MAC_SendToPhy:
        eventStr = "MacSendToLower";
        break;
    case MAC_ReceiveFromPhy:
        eventStr = "MacReceiveFromLower";
        break;
    case MAC_Drop:
        eventStr = "MacDrop";
        break;
    case MAC_SendToUpper:
        eventStr = "MacSendToUpper";
        break;
    case MAC_ReceiveFromUpper:
        eventStr = "MacReceiveFromUpper";
        break;

    default:
        ERROR_Assert(FALSE, "Unknown event type in HandleMacDBEvents.");
    }
    std::string msgId;

    StatsDBMappingParam *mapParamInfo = (StatsDBMappingParam*)
        MESSAGE_ReturnInfo( msg, INFO_TYPE_StatsDbMapping);

     if (mapParamInfo == NULL)
    {
       StatsDBAddMessageMsgIdIfNone(node, msg);
        mapParamInfo = (StatsDBMappingParam*)
            MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsDbMapping);
    }
    msgId = mapParamInfo->msgId;
    int msgSize = 0;

#ifdef ADDON_BOEINGFCS
    MacCesUsapDBATruePacketInfo* truePacketInfo =
            (MacCesUsapDBATruePacketInfo*)
             MESSAGE_ReturnInfo(msg,
                                INFO_TYPE_MacCesUsapTruePacketInfo);
    if (truePacketInfo)
    {
        msgSize = truePacketInfo->getTruePacketSize();
    }
    else
    {
#endif
        if (!msg->isPacked)
        {
            msgSize = MESSAGE_ReturnPacketSize(msg);
        }
        else
        {
            msgSize = MESSAGE_ReturnActualPacketSize(msg);
        }
#ifdef ADDON_BOEINGFCS
    }
#endif
    StatsDBMacEventParam macParam(node->nodeId, msgId, interfaceIndex,
            msgSize, eventStr);

    macParam.SetMsgSeqNum(msg->sequenceNumber);

    int channelIndex = 0;
#ifdef ADDON_BOEINGFCS
    if (protocol != MAC_PROTOCOL_CES_WINTNCW &&
        protocol != MAC_PROTOCOL_CES_WINTGBS &&
        protocol != MAC_PROTOCOL_802_3 &&
        protocol != MAC_PROTOCOL_LINK)
#else
    if (protocol != MAC_PROTOCOL_802_3 &&
        protocol != MAC_PROTOCOL_LINK)
#endif
    {
        PHY_GetTransmissionChannel(node, phyIndex, &channelIndex);
        macParam.SetChannelIndex(channelIndex);
    }

    if (failureTypeSpecifed)
    {
        macParam.SetFailureType(failureType);
    }
    BOOL isCtrlFrame = FALSE;
    BOOL isMyFrame = FALSE;
    BOOL isAnyFrame = FALSE;
    BOOL isAggregated = FALSE;

    if (eventType == MAC_SendToUpper
        || eventType == MAC_ReceiveFromUpper
        || (eventType == MAC_Drop && processMacHeader == FALSE))
    {
        macParam.SetHdrSize(0);
        macParam.SetFrameType("DATA");
    }
    else
    {
        switch(protocol)
        {
            case MAC_PROTOCOL_DOT11:
            {
                // return headersize, frametype
                MacDot11GetPacketProperty(node,
                    msg,
                    interfaceIndex,
                    eventType,
                    macParam,
                    isMyFrame,
                    isAnyFrame);
                break;
            }
            case MAC_PROTOCOL_LINK:
            {
                MacLinkGetPacketProperty(node,
                    msg,
                    interfaceIndex,
                    eventType,
                    macParam,
                    isMyFrame,
                    isAnyFrame);
                break;
            }
            case MAC_PROTOCOL_802_3:
            {
                Mac802_3GetPacketProperty(node,
                    msg,
                    interfaceIndex,
                    eventType,
                    macParam,
                    isMyFrame,
                    isAnyFrame);
                break;
            }
#ifdef LTE_LIB
            case MAC_PROTOCOL_LTE:
            {
                LteLayer2GetPacketProperty(
                    node,
                    msg,
                    interfaceIndex,
                    eventType,
                    macParam,
                    isMyFrame);
                break;
            }
#endif // LTE_LIB
#ifdef ADDON_BOEINGFCS
            case MAC_PROTOCOL_CES_WINTNCW:
            {
                MacCesWintNcwGetPacketPropertyForMacEvent(
                    node,
                    msg,
                    interfaceIndex,
                    macParam,
                    isMyFrame,
                    isAnyFrame);
                break;
            }
            case MAC_PROTOCOL_CES_WINTHNW:
            {
                MacCesWintHnwGetPacketPropertyForMacEvent(
                    node,
                    msg,
                    interfaceIndex,
                    macParam,
                    isMyFrame,
                    isAnyFrame);
                break;
            }
            case MAC_PROTOCOL_CES_WINTGBS:
            {
                MacCesWintGBSGetPacketPropertyForMacEvent(
                    node,
                    msg,
                    interfaceIndex,
                    macParam,
                    isMyFrame,
                    isAnyFrame);
                break;
            }
            case MAC_PROTOCOL_CES_SRW_PORT:
            {
                ERROR_ReportError("need to implement");
                break;
            }
            case MAC_PROTOCOL_CES_WNW_MDL:
            {
                MacCesUsapDBAGetPacketPropertyForMacEvent(
                    node,
                    msg,
                    interfaceIndex,
                    macParam,
                    isCtrlFrame,
                    isMyFrame,
                    isAnyFrame);
                break;
            }
            case MAC_PROTOCOL_CES_SINCGARS:
            {
                MacCesSincgarsGetPacketPropertyForMacEvent(
                    node,
                    msg,
                    interfaceIndex,
                    macParam,
                    isCtrlFrame,
                    isMyFrame,
                    isAnyFrame);
                break;
            }
            case MAC_PROTOCOL_CES_EPLRS:
            {
                MacCesEplrsGetPacketPropertyForMacEvent(
                    node,
                    msg,
                    interfaceIndex,
                    macParam,
                    isCtrlFrame,
                    isMyFrame,
                    isAnyFrame);
                break;
            }
#endif
            default:
            {
                ERROR_Assert(FALSE,
                         "Unknown protocol in HandleMacDBEvents.");
            }
        }
    }

        if (eventType == MAC_ReceiveFromPhy && !isMyFrame && !isAnyFrame)
        {
            return;
        }
        STATSDB_HandleMacEventsTableInsert(node, NULL, macParam);
}

//--------------------------------------------------------------------------
// FUNCTION: HandleMacDBSummary
// PURPOSE:  to process the Stats DB summary input.
// PARAMETERS
// + node : Node* : Pointer to a node
// + msg : Message*: pointer to the frame.
// RETURN Pointer to the data after skipping headers.
//--------------------------------------------------------------------------
void HandleMacDBSummary(Node* node,
    Message* msg,
    int interfaceIndex,
    MacDBEventType eventType,
    int headerSize,
    BOOL isCtrlFrame,
    MAC_PROTOCOL protocol,
    BOOL isMyFrame,
    BOOL isAnyFrame)
{

    MacData* mac = (MacData *) node->macData[interfaceIndex];
    ERROR_Assert(mac , "Error in StatsDb, Mac Data Unavailable\n");

    StatsDBMacSummary * macSummary = mac->summaryStats ;

    if (eventType == MAC_SendToPhy)
    {
        if (isCtrlFrame)
        {
            ++macSummary->controlFramesSent;
            macSummary->controlBytesSent += MESSAGE_ReturnPacketSize(msg);
        }else
        {
            if (isAnyFrame)
            {
                ++macSummary->bcast_dataFramesSent;
                macSummary->bcast_dataBytesSent += MESSAGE_ReturnPacketSize(msg);
            }else {
                ++macSummary->ucast_dataFramesSent;
                macSummary->ucast_dataBytesSent += MESSAGE_ReturnPacketSize(msg);
            }
        }

        MacSummaryInfo* macSummaryInfo = (MacSummaryInfo*)
            MESSAGE_ReturnInfo(msg, INFO_TYPE_MacSummaryStats);

        if (macSummaryInfo == NULL)
        {
            MESSAGE_AddInfo(node, msg, sizeof(MacSummaryInfo),
                    INFO_TYPE_MacSummaryStats);
            macSummaryInfo = (MacSummaryInfo*)MESSAGE_ReturnInfo(msg,
                INFO_TYPE_MacSummaryStats);
        }
        macSummaryInfo->macSummaryPktSendTime = node->getNodeTime();
        macSummaryInfo->senderId = node->nodeId;
        macSummaryInfo->interfaceIndex = interfaceIndex;

    }
    else if (eventType == MAC_ReceiveFromPhy)
    {
        if (isCtrlFrame)
        {
            ++macSummary->controlFramesReceived;
            macSummary->controlBytesReceived += MESSAGE_ReturnPacketSize(msg);
        }
        else
        {
            if (isAnyFrame)
            {
                ++macSummary->bcast_dataFramesReceived;
                macSummary->bcast_dataBytesReceived += MESSAGE_ReturnPacketSize(msg);
            }else if (isMyFrame){
                ++macSummary->ucast_dataFramesReceived;
                macSummary->ucast_dataBytesReceived += MESSAGE_ReturnPacketSize(msg);
            }
            else
                ERROR_Assert(FALSE, "Unkown type in HandleMacDBSummary.");

        }

        // Calculate the delay and jitter.

        MacSummaryInfo *macSummaryInfo = (MacSummaryInfo*)
            MESSAGE_ReturnInfo(msg, INFO_TYPE_MacSummaryStats);

        ERROR_Assert(macSummaryInfo, "ERROR in HandleMacDBSummary.");

        clocktype delay = node->getNodeTime() -
            macSummaryInfo->macSummaryPktSendTime;

        ERROR_Assert(delay > 0,
            "ERROR of delay calculation in HandleMacDBSummary.");

        // Jitter can only be measured after receiving
        // two packets.

        pair<int, int> key = pair<int, int>(
                macSummaryInfo->senderId,
                macSummaryInfo->interfaceIndex) ;
        map<pair<int, int>, pair<clocktype, UInt64> >::iterator iter
            = macSummary->perSourceInfo.find(key) ;

        if (iter == macSummary->perSourceInfo.end())
        {
            pair<clocktype, UInt64> value = pair<clocktype, UInt64>
                (0, 0) ;
            macSummary->perSourceInfo.insert(pair<pair<int, int>,
                pair<clocktype, UInt64> >(key, value)) ;
            iter = macSummary->perSourceInfo.find(key) ;
        }
        ++iter->second.second  ;
        int packetRcvd = iter->second.second ;

        if (packetRcvd > 1)
        {
            // delay - lastDelayTime
            clocktype tempJitter = delay - iter->second.first;

            if (tempJitter < 0) {
              tempJitter = -tempJitter;
            }
            macSummary->totalJitter += static_cast<clocktype>(
                 ((double)tempJitter - macSummary->totalJitter) / 16.0);
        }
        iter->second.first = delay;

        macSummary->totalDelay += delay;
    }
    else if (eventType == MAC_Drop)
    {

        ++macSummary->FramesDropped;
        macSummary->BytesDropped += MESSAGE_ReturnPacketSize(msg);
    }
    else if (eventType == MAC_SendToUpper)
    {

    }
    else if (eventType == MAC_ReceiveFromUpper)
    {

    }
    else
        ERROR_Assert(FALSE,
        "Unknown eventType in HandleMacDBSummary.");
}

//--------------------------------------------------------------------------
// FUNCTION: HandleMacDBSummary
// PURPOSE:  to process the Stats DB summary input.
// PARAMETERS
// + node : Node* : Pointer to a node
// + msg : Message*: pointer to the frame.
// RETURN Pointer to the data after skipping headers.
//--------------------------------------------------------------------------
void HandleMacDBSummary(
    Node *node,
    Message* msg,
    int interfaceIndex,
    MacDBEventType eventType,
    MAC_PROTOCOL protocol)
{
    // Insert on to the table
    // Check first if the table exists.
    StatsDb* db = node->partitionData->statsDb;

    if (!db || !db->statsSummaryTable->createMacSummaryTable)
    {
        // Table does not exist.
        return;
    }

    int frameSize = 0;
    BOOL isCtrlFrame = FALSE;
    int headerSize = 0;
    std::string dstAddrStr;
    BOOL isMyFrame = FALSE;
    BOOL isAnyFrame = FALSE;

    switch(protocol)
    {
    case MAC_PROTOCOL_DOT11:
        // return headersize, frametype
        MacDot11GetPacketProperty(node, msg, interfaceIndex,
            headerSize, dstAddrStr, isCtrlFrame, isMyFrame,
            isAnyFrame);
        break;
    // other mac protocol
#ifdef ADDON_BOEINGFCS
    case MAC_PROTOCOL_CES_WINTNCW:
        MacCesWintNcwGetPacketProperty(node, msg, interfaceIndex,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_WINTHNW:
        MacCesWintHnwGetPacketProperty(node, msg, interfaceIndex,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_WINTGBS:
        MacCesWintGBSGetPacketProperty(node, msg, interfaceIndex,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_SRW_PORT:
        ERROR_ReportError("need to implement");
        break;
    case MAC_PROTOCOL_CES_WNW_MDL:
        MacCesUsapDBAGetPacketProperty(
            node,
            msg,
            interfaceIndex,
            isCtrlFrame,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_SINCGARS:
        MacCesSincgarsGetPacketProperty(
            node,
            msg,
            interfaceIndex,
            isCtrlFrame,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_EPLRS:
        MacCesEplrsGetPacketProperty(
            node,
            msg,
            interfaceIndex,
            isCtrlFrame,
            isMyFrame,
            isAnyFrame);
        break;
#endif
    default:
        return;
        //ERROR_Assert(FALSE, "Unknown protocol in HandleMacDBEvents.");
    }

    if (eventType == MAC_ReceiveFromPhy && !isMyFrame && !isAnyFrame)
    {
        return ;
    }

    HandleMacDBSummary(
        node, msg, interfaceIndex,
        eventType, headerSize, isCtrlFrame, protocol, isMyFrame, isAnyFrame);
}

//--------------------------------------------------------------------------
// FUNCTION: HandleMacDBAggregate
// PURPOSE:  to process the Stats DB summary input.
// PARAMETERS
// + node : Node* : Pointer to a node
// + msg : Message*: pointer to the frame.
// RETURN Pointer to the data after skipping headers.
//--------------------------------------------------------------------------
void HandleMacDBAggregate(Node* node,
    Message* msg,
    int interfaceIndex,
    MacDBEventType eventType,
    int headerSize,
    BOOL isCtrlFrame,
    MAC_PROTOCOL protocol,
    BOOL isMyFrame,
    BOOL isAnyFrame)
{

    MacData* mac = (MacData *) node->macData[interfaceIndex];
    ERROR_Assert(mac , "Error in StatsDb, Mac Data Unavailable\n");

    StatsDBMacAggregate * macAggregate = mac->aggregateStats ;

    if (eventType == MAC_SendToPhy)
    {
        if (isCtrlFrame)
        {
            ++macAggregate->controlFramesSent;
            macAggregate->controlBytesSent += MESSAGE_ReturnPacketSize(msg);
        }else
        {
            ++macAggregate->dataFramesSent;
            macAggregate->dataBytesSent += MESSAGE_ReturnPacketSize(msg);
        }

        MacSummaryInfo* macAggregateInfo = (MacSummaryInfo*)
            MESSAGE_ReturnInfo(msg, INFO_TYPE_MacSummaryStats);

        if (macAggregateInfo == NULL)
        {
            MESSAGE_AddInfo(node, msg, sizeof(MacSummaryInfo),
                    INFO_TYPE_MacSummaryStats);
            macAggregateInfo = (MacSummaryInfo*)MESSAGE_ReturnInfo(msg,
                INFO_TYPE_MacSummaryStats);
        }
        macAggregateInfo->macSummaryPktSendTime = node->getNodeTime();
        macAggregateInfo->senderId = node->nodeId ;
        macAggregateInfo->interfaceIndex = interfaceIndex ;
    }
    else if (eventType == MAC_ReceiveFromPhy)
    {
        if (isCtrlFrame)
        {
            ++macAggregate->controlFramesReceived;
            macAggregate->controlBytesReceived += MESSAGE_ReturnPacketSize(msg);
        }
        else
        {
            ++macAggregate->dataFramesReceived;
            macAggregate->dataBytesReceived += MESSAGE_ReturnPacketSize(msg);
        }

        // Calculate the delay and jitter.

        MacSummaryInfo *macAggregateInfo = (MacSummaryInfo*)
            MESSAGE_ReturnInfo(msg, INFO_TYPE_MacSummaryStats);

        ERROR_Assert(macAggregateInfo, "ERROR in HandleMacDBAggregate.");

        clocktype delay = node->getNodeTime() -
            macAggregateInfo->macSummaryPktSendTime;

        ERROR_Assert(delay > 0,
            "ERROR of delay calculation in HandleMacDBAggregate.");

        // Jitter can only be measured after receiving
        // two packets.

        pair<int, int> key = pair<int, int>(
                macAggregateInfo->senderId,
                macAggregateInfo->interfaceIndex) ;
        map<pair<int, int>, pair<clocktype, UInt64> >::iterator iter
            = macAggregate->perSourceInfo.find(key) ;

        if (iter == macAggregate->perSourceInfo.end())
        {
            pair<clocktype, UInt64> value = pair<clocktype, UInt64>
                (0, 0) ;
            macAggregate->perSourceInfo.insert(pair<pair<int, int>,
                pair<clocktype, UInt64> >(key, value)) ;
            iter = macAggregate->perSourceInfo.find(key) ;
        }
        ++iter->second.second  ;
        int packetRcvd = iter->second.second ;

        if (packetRcvd > 1)
        {
            // delay - lastDelayTime
            clocktype tempJitter = delay - iter->second.first;

            if (tempJitter < 0) {
              tempJitter = -tempJitter;
            }
            macAggregate->totalJitter += static_cast<clocktype>(
                 ((double)tempJitter - macAggregate->totalJitter) / 16.0);
        }
        iter->second.first = delay;
        macAggregate->totalDelay += delay;
    }
    else if (eventType == MAC_Drop)
    {
    }
    else if (eventType == MAC_SendToUpper)
    {

    }
    else if (eventType == MAC_ReceiveFromUpper)
    {

    }
    else
        ERROR_Assert(FALSE,
        "Unknown eventType in HandleMacDBAggregate.");

}
//--------------------------------------------------------------------------
// FUNCTION: HandleMacDBAggregate
// PURPOSE:  to process the Stats DB aggregate input.
// PARAMETERS
// + node : Node* : Pointer to a node
// + msg : Message*: pointer to the frame.
// RETURN Pointer to the data after skipping headers.
//--------------------------------------------------------------------------
void HandleMacDBAggregate(
    Node *node,
    Message* msg,
    int interfaceIndex,
    MacDBEventType eventType,
    MAC_PROTOCOL protocol)
{
    // Insert on to the table
    // Check first if the table exists.
    StatsDb* db = node->partitionData->statsDb;

    if (!db || !db->statsAggregateTable->createMacAggregateTable)
    {
        // Table does not exist.
        return;
    }

    int frameSize = 0;
    BOOL isCtrlFrame = FALSE;
    int headerSize = 0;
    std::string dstAddrStr;
    BOOL isMyFrame = FALSE;
    BOOL isAnyFrame = FALSE;

    switch(protocol)
    {
    case MAC_PROTOCOL_DOT11:
        // return headersize, frametype
        MacDot11GetPacketProperty(node, msg, interfaceIndex,
            headerSize, dstAddrStr, isCtrlFrame, isMyFrame,
            isAnyFrame);
        break;
#ifdef ADDON_BOEINGFCS
    case  MAC_PROTOCOL_CES_SRW_PORT:
        ERROR_ReportError("need to implement");
        break;
    case MAC_PROTOCOL_CES_WINTNCW:
        MacCesWintNcwGetPacketProperty(node, msg, interfaceIndex,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_WINTHNW:
        MacCesWintHnwGetPacketProperty(node, msg, interfaceIndex,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_WINTGBS:
        MacCesWintGBSGetPacketProperty(node, msg, interfaceIndex,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_WNW_MDL:
        MacCesUsapDBAGetPacketProperty(
            node,
            msg,
            interfaceIndex,
            isCtrlFrame,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_SINCGARS:
        MacCesSincgarsGetPacketProperty(
            node,
            msg,
            interfaceIndex,
            isCtrlFrame,
            isMyFrame,
            isAnyFrame);
        break;
    case MAC_PROTOCOL_CES_EPLRS:
        MacCesEplrsGetPacketProperty(
            node,
            msg,
            interfaceIndex,
            isCtrlFrame,
            isMyFrame,
            isAnyFrame);
        break;
#endif
    // other mac protocol
    default:
        return;
        //ERROR_Assert(FALSE, "Unknown protocol in HandleMacDBEvents.");
    }

    if (eventType == MAC_ReceiveFromPhy && !isMyFrame && !isAnyFrame)
    {
        return ;
    }

    HandleMacDBAggregate(
        node, msg, interfaceIndex,
        eventType, headerSize, isCtrlFrame, protocol, isMyFrame, isAnyFrame);
}
void HandleMacDBConnectivity(Node *node, int interfaceIndex,
                             Message *msg, MacDBEventType eventType)
{

    // Insert on to the table
    // Check first if the table exists.
    StatsDb* db = node->partitionData->statsDb;
    if (!db || !db->statsConnTable->createMacConnTable)
    {
        // Table does not exist.
        return;
    }

    if (eventType == MAC_ReceiveFromPhy)
    {
        // get the info field

        StatsDBMacConnParam * macConnInfo =
            (StatsDBMacConnParam*)
            MESSAGE_ReturnInfo(msg, INFO_TYPE_MacConnStats);

        if (macConnInfo == NULL)
        {
            ERROR_ReportWarning("StatsDBMacConnParam info field is empty.");
            return ;
        }

        StatsDBConnTable::macConnTableElement *pConnElement = NULL;
        std::pair<int, int> key =std::pair<int, int>(
            macConnInfo->m_SenderId, node->nodeId);

        std::pair<StatsDBConnTable::MacConnTable_Iterator,
            StatsDBConnTable::MacConnTable_Iterator> pair_iter =
            db->statsConnTable->MacConnTable.equal_range(key) ;

        StatsDBConnTable::MacConnTable_Iterator iter = pair_iter.first;

        for (; iter != pair_iter.second; ++iter)
        {
            if (iter->second->channelIndex == macConnInfo->m_ChannelIndex &&
                iter->second->interfaceIndex == macConnInfo->m_InterfaceIndex)
            {
                pConnElement = iter->second;
                break ;
            }
        }
        if (iter == pair_iter.second)
        {
            pConnElement = new StatsDBConnTable::macConnTableElement(TRUE,
                macConnInfo->channelIndex_str, macConnInfo->m_ChannelIndex);
            pConnElement->interfaceIndex = macConnInfo->m_InterfaceIndex;
            db->statsConnTable->MacConnTable.insert(
                pair<pair<int, int>, StatsDBConnTable::macConnTableElement *>(key, pConnElement));
        }
        //printf("Mac Conn insert sender %d [%d] to receverId %d on channel %d \n",
        //    macConnInfo->m_SenderId, macConnInfo->m_InterfaceIndex,
        //    node->nodeId, macConnInfo->m_ChannelIndex);

    }else if (eventType == MAC_SendToPhy)
    {
        StatsDBMacConnParam * macConnInfo =
            (StatsDBMacConnParam*)
            MESSAGE_ReturnInfo(msg, INFO_TYPE_MacConnStats);

        if (macConnInfo == NULL)
        {
            // Info does not exist. Insert one.
            macConnInfo =
                (StatsDBMacConnParam*) MESSAGE_AddInfo(
                              node,
                              msg,
                              sizeof(StatsDBMacConnParam),
                              INFO_TYPE_MacConnStats);
        }

        macConnInfo->m_SenderId = node->nodeId;
        macConnInfo->m_InterfaceIndex = interfaceIndex;

        if (
#ifdef ADDON_BOEINGFCS
            node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WINTNCW ||
            node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CES_WINTGBS ||
#endif
            node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_SATCOM ||
            node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_LINK ||
            node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_802_3)
        {
            macConnInfo->m_ChannelIndex = -1 ;
        }
        else
        {
            PHY_GetTransmissionChannel(node,
                node->macData[interfaceIndex]->phyNumber,
                &macConnInfo->m_ChannelIndex);
        }
        std::string channelIndex_str =
            STATSDB_ChannelToString(node, interfaceIndex,
            macConnInfo->m_ChannelIndex);
        strcpy(macConnInfo->channelIndex_str,
            channelIndex_str.c_str());
    }
    else ERROR_Assert(FALSE, "Error in HandleMacDBConnectivity.");
}

#endif
