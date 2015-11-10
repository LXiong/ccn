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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "network_ip.h"
#include "ipv6.h"
#include "network_dualip.h"
#include "mac_arp.h"
#include "stats_net.h"
#include "partition.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#ifdef WIRELESS_LIB
#include "routing_iarp.h"
#include "routing_zrp.h"
#include "routing_iarp.h"
#include "routing_ierp.h"
#include "cellular_layer3.h"

#ifdef CELLULAR_LIB
#include "layer3_gsm.h"
#endif
#endif // WIRELESS_LIB

#ifdef CYBER_LIB
#include "crypto.h"

#ifdef DO_ECC_CRYPTO
MPI caEccKey[12];
#endif // DO_ECC_CRYPTO
static BOOL caNeedInitialization = TRUE;
#endif // CYBER_LIB

#ifdef ADDON_BOEINGFCS
// From network_ip.h
#include "network_ces_inc.h"
#include "mi_ces_nbr_mon.h"

#include "boeingfcs_network.h"
#include "mode_ces_wnw_receive_only.h"
#endif

#ifdef CYBER_CORE
#include "network_pki_header.h"
#endif // CYBER_CORE
//InsertPatch NETWORK_ROUTING_HEADER

//-----------------------------------------------------------------------------
// DEFINES (none)
//----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// STRUCTS, ENUMS, AND TYPEDEFS (none)
//----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PROTOTYPES FOR FUNCTIONS WITH INTERNAL LINKAGE (none)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FUNCTIONS WITH EXTERNAL LINKAGE
//-----------------------------------------------------------------------------

// Compare two addresses
bool Address::operator< (const Address& rhs) const
{
    if (networkType != rhs.networkType)
    {
        // The below warning message is expected in DUAL-IP
        // scenarios, hence it is ignored.
        // ERROR_ReportWarning("Comparing two different addresses");
        return false;
    }
    else if (networkType == NETWORK_IPV4)
    {
        return interfaceAddr.ipv4 < rhs.interfaceAddr.ipv4;
    }
    else if (networkType == NETWORK_IPV6)
    {
        return memcmp(            
            //interfaceAddr.ipv6.in6_u.u6_addr8,
            //rhs.interfaceAddr.ipv6.in6_u.u6_addr8,
            &interfaceAddr.ipv6,
            &rhs.interfaceAddr.ipv6,
            16) < 0;
    }
    else
    {
        ERROR_ReportWarning("Unsupported address type");
        return false;
    }
}

// Compare two addresses
bool Address::operator> (const Address& rhs) const
{
    if (networkType != rhs.networkType)
    {
        // The below warning message is expected in DUAL-IP
        // scenarios, hence it is ignored.
        // ERROR_ReportWarning("Comparing two different addresses");
        return false;
    }
    else if (networkType == NETWORK_IPV4)
    {
        return interfaceAddr.ipv4 > rhs.interfaceAddr.ipv4;
    }
    else if (networkType == NETWORK_IPV6)
    {
        return memcmp(            
            //interfaceAddr.ipv6.in6_u.u6_addr8,
            //rhs.interfaceAddr.ipv6.in6_u.u6_addr8,
            &interfaceAddr.ipv6,
            &rhs.interfaceAddr.ipv6,
            16) > 0;
    }
    else
    {
        ERROR_ReportWarning("Unsupported address type");
        return false;
    }
}

//-----------------------------------------------------------------------------
// Init functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FUNCTION     NETWORK_PreInit()
// PURPOSE      Network-layer initialization required before any of the
//              other layers are initialized.
// PARAMETERS   Node *node
//                  Pointer to node.
//              const NodeInput *nodeInput
//                  Pointer to node input.
//
// NOTES        This function calls NetworkIpPreInit().  See the
//              comments for that function.
//-----------------------------------------------------------------------------

void
NETWORK_PreInit(Node *node, const NodeInput *nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

#ifdef CYBER_LIB
#ifdef DO_ECC_CRYPTO
    char buffer[MAX_STRING_LENGTH];
    char keyFileName[MAX_STRING_LENGTH], pubKeyFileName[MAX_STRING_LENGTH];
    FILE *keyFile, *pubKeyFile;
    int i;

    if (caNeedInitialization)
    {
        // Certificate Authority (CA)'s ECC public/private key pair handling:
        // (1) Create one if there is none.
        // (2) Test its validity if the file is already there.
        sprintf(keyFileName, "default.privatekey.certificate_authority");
        sprintf(pubKeyFileName, "default.publickey.certificate_authority");

        keyFile = fopen(keyFileName, "r");
        pubKeyFile = fopen(pubKeyFileName, "r");
        if (keyFile == NULL || pubKeyFile == NULL)
        {
            if (keyFile == NULL)
            {
                // Certificate Authority's private key doesn't exist, create it
                keyFile = fopen(keyFileName, "w");
                sprintf(buffer, "Error in creating key file %s\n", keyFileName);
                ERROR_Assert(keyFile != NULL, buffer);
            }
            if (pubKeyFile == NULL)
            {
                // Certificate Authority's public key doesn't exist, create it
                pubKeyFile = fopen(pubKeyFileName, "w");
                sprintf(buffer, "Error in creating public key file %s\n", pubKeyFileName);
                ERROR_Assert(pubKeyFile != NULL, buffer);
            }

            int invalid = 1;

            while (invalid)
            {
                fprintf(stderr, "Generate Elliptic Curve Cryptosystem (ECC) key for Certificate Authority (CA).\n");

                // Choose the random nonce k
                caEccKey[11] = EccChooseRandomNumber(NULL, QUALNET_ECC_KEYLENGTH);
                invalid = ecc_generate(PUBKEY_ALGO_ECC,
                                       QUALNET_ECC_KEYLENGTH,
                                       caEccKey,
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
                mpi_print(pubKeyFile, caEccKey[i], 1);
                fputc('\n', pubKeyFile);
            }
            // key has all 11 elements
            for (i=0; i<11; i++)
            {
                fprintf(keyFile, "0x");
                mpi_print(keyFile, caEccKey[i], 1);
                fputc('\n', keyFile);
            }
        }
        else
        {
            // The pre-generated key is already in the file,
            // use it rather than generate it.  This saves simulation time.
            for (i=0; i<11; i++)
            {
                fgets(buffer, QUALNET_MAX_STRING_LENGTH, keyFile);
                if (buffer[strlen(buffer)-1] != '\n')
                {
                    sprintf(buffer,
                            "Key file %s is corrupted. Please delete it.\n",
                            keyFileName);
                    ERROR_ReportError(buffer);
                }
                buffer[strlen(buffer)-1] = '\0';

                caEccKey[i] = mpi_alloc_set_ui(0);
                mpi_fromstr(caEccKey[i], buffer);
            }
            // Choose the random nonce k
            caEccKey[11] =
                EccChooseRandomNumber(caEccKey[6]/*sk.n*/,
                                      QUALNET_ECC_KEYLENGTH);
            int invalid = test_ecc_key(PUBKEY_ALGO_ECC,
                                       caEccKey,
                                       QUALNET_ECC_KEYLENGTH);
            fprintf(stderr, "Certificate Authority (CA)'s ECC key is %s.\n",
                    invalid?"INVALID":"VALID");
            while (invalid)
            {
                fprintf(stderr, "Generate Elliptic Curve Cryptosystem (ECC) key for Certificate Authority (CA).\n");
                // Choose the random nonce k
                mpi_free(caEccKey[11]);
                caEccKey[11] = EccChooseRandomNumber(NULL, QUALNET_ECC_KEYLENGTH);
                invalid = ecc_generate(PUBKEY_ALGO_ECC,
                                       QUALNET_ECC_KEYLENGTH,
                                       caEccKey,
                                       NULL);
            }
        }
        fflush(stderr);
        fclose(keyFile);
        fclose(pubKeyFile);

        caNeedInitialization = FALSE;
    } // if (caNeedInitialization)
#endif // DO_ECC_CRYPTO
#endif // CYBER_LIB
    node->hostname = (char*)MEM_malloc(sizeof(char) * MAX_STRING_LENGTH);
    sprintf(node->hostname, "host%d", node->nodeId);
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "HOSTNAME",
        &retVal,
        node->hostname);

    // ATM : Check if it is a ATM Node
    node->adaptationData.endSystem = FALSE;
    node->adaptationData.genlSwitch = FALSE;
    IO_ReadString(
        node,
        node->nodeId,
        nodeInput,
        "ATM-NODE",
        &retVal,
        buf);

    if (retVal && !strcmp(buf, "YES"))
    {
        retVal = FALSE;

        // check if it is an END SYSTEM
        IO_ReadString(
            node,
            node->nodeId,
            nodeInput,
            "ATM-END-SYSTEM",
            &retVal,
            buf);

        if (retVal && !strcmp(buf, "YES"))
        {
            // By default IP is used
            node->networkData.networkProtocol = IPV4_ONLY;
            node->adaptationData.endSystem = TRUE;
            node->adaptationData.genlSwitch = FALSE;
            NetworkIpPreInit(node);
        }
        else
        {
            node->adaptationData.endSystem = FALSE;
            node->adaptationData.genlSwitch = TRUE;
        }
    }
    else
    {
        node->networkData.networkProtocol =
            MAPPING_GetNetworkProtocolTypeForNode(node, node->nodeId);

        switch (node->networkData.networkProtocol)
        {
            case IPV4_ONLY:
            {
                NetworkIpPreInit(node);
                break;
            }
            case IPV6_ONLY:
            {
                Ipv6PreInit(node);
                break;
            }
            case DUAL_IP:
            {
                NetworkIpPreInit(node);
                Ipv6PreInit(node);
                break;
            }
            case GSM_LAYER3:
#ifdef CELLULAR_LIB
            {
                GsmLayer3Data *nwGsmData;

                NetworkIpPreInit(node);
                nwGsmData = (GsmLayer3Data *)MEM_malloc(sizeof(GsmLayer3Data));
                memset(nwGsmData,
                       0,
                       sizeof(GsmLayer3Data));
                node->networkData.gsmLayer3Var = (GsmLayer3Data *) nwGsmData;
                break;
            }
#else
            {
                ERROR_ReportMissingLibrary(
                    "GSM-LAYER3", "CELLULAR");
            }
            break;
#endif
            case CELLULAR:
#ifdef WIRELESS_LIB
            {
                CellularLayer3Data *nwCellularLayer3Data;

                NetworkIpPreInit(node);
                nwCellularLayer3Data = (CellularLayer3Data *) MEM_malloc(sizeof(CellularLayer3Data));
                memset(nwCellularLayer3Data,
                       0,
                      sizeof(CellularLayer3Data));
                node->networkData.cellularLayer3Var = (CellularLayer3Data *) nwCellularLayer3Data;
                CellularLayer3PreInit(node,nodeInput);
                break;
            }
#else
            {
                ERROR_ReportMissingLibrary(
                    "CELLULAR-UNIVERSAL", "WIRELESS");
            }
            break;
#endif // WIRELESS_LIB
            default:
            {
                ERROR_ReportError("NETWORK-PROTOCOL parameter must be IP,"
                    " IPv6, DUAL_IP or CELLULAR_LAYER3 or GSM-LAYER3");
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// FUNCTION     NETWORK_Initialize()
// PURPOSE      Initialization function for network layer.
//              Initializes IP.
// PARAMETERS   Node *node
//                  Pointer to node.
//              const NodeInput *nodeInput
//                  Pointer to node input.
//-----------------------------------------------------------------------------

void
NETWORK_Initialize(Node *node, const NodeInput *nodeInput)
{
    BOOL retVal = FALSE;
    BOOL retTrue = FALSE;

    node->networkData.isArpEnable = FALSE;
    
    IO_ReadBool(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "NETWORK-LAYER-STATISTICS",
        &retVal,
        &retTrue);

    if (retVal == FALSE || !retTrue)
    {
        node->networkData.networkStats = FALSE;
    }
    else if (retTrue)
    {
        node->networkData.networkStats = TRUE;
    }
    else
    {
        ERROR_ReportWarning(
            "Expecting YES or NO for NETWORK-PROTOCOL-STATISTICS parameter");
        node->networkData.networkStats = FALSE;
    }
#ifdef ADDON_DB
    if (!node->networkData.networkStats)
    {
        if (node->partitionData->statsDb)
        {
            if (node->partitionData->statsDb->statsAggregateTable->createNetworkAggregateTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB NETWORK_Aggregate table requires\n"
                    " NETWORK-LAYER-STATISTICS to be set to YES\n");
            }
            if (node->partitionData->statsDb->statsSummaryTable->createNetworkSummaryTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB NETWORK_Summary table requires\n"
                    " NETWORK-LAYER-STATISTICS to be set to YES\n");
            }
        }
    }
#endif
    NetworkProtocolType networkProtocol =
        NetworkIpGetNetworkProtocolType(node, node->nodeId);

#ifdef ADDON_BOEINGFCS
    //Intranet Layer INC
    RoutingCesIncInit(node, nodeInput);
#endif

    switch (networkProtocol)
    {
        case IPV4_ONLY:
        {
            NetworkIpInit(node, nodeInput);
            if (!node->switchData)
            {
                IpRoutingInit(node, nodeInput);
                NetworkIpMibsInit(node);
            }
            break;
        }
        case IPV6_ONLY:
        {
            IPv6Init(node, nodeInput);
            if (!node->switchData)
            {
                IPv6RoutingInit(node, nodeInput);
            }
            break;
        }
        case DUAL_IP:
        {
            NetworkIpInit(node, nodeInput);
            IPv6Init(node, nodeInput);
            if (!node->switchData)
            {
                TunnelInit(node, nodeInput);
                IpRoutingInit(node, nodeInput);
                IPv6RoutingInit(node, nodeInput);
            }
            break;
        }
#ifdef CELLULAR_LIB
        case GSM_LAYER3:
        {
            NetworkIpInit(node, nodeInput);
            GsmLayer3Init(node, nodeInput);
            IpRoutingInit(node, nodeInput);
            break;
        }
#endif // CELLULAR_LIB
#ifdef WIRELESS_LIB
        case CELLULAR:
        {
            NetworkIpInit(node, nodeInput);
            CellularLayer3Init(node, nodeInput);
            IpRoutingInit(node, nodeInput);
            break;
        }
#endif
        default:
        {
#ifdef CELLULAR_LIB
            ERROR_ReportError("NETWORK-PROTOCOL parameter must be IP,"
                " IPv6, DUAL_IP or CELLULAR_LAYER3 or GSM-LAYER3");
#endif // CELLULAR_LIB
            break;
        }
    }

    if (node->networkData.networkVar)
    {
        NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
        if (ip && networkProtocol != IPV6_ONLY)
        {
            ip->newStats = new STAT_NetStatistics(node);
        }
    }

    ArpInit(node, nodeInput);

#ifdef CYBER_CORE
    BOOL readValue;
    IO_ReadBool(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "PKI-ENABLED",
        &retVal,
        &readValue);

    if (retVal == TRUE && readValue)
    {
        PKIInitialize(node, nodeInput);
    }
#endif // CYBER_CORE
}

//-----------------------------------------------------------------------------
// Layer function
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FUNCTION     NETWORK_ProcessEvent()
// PURPOSE      Calls layer function of IP.
// PARAMETERS   Node *node
//                  Pointer to node.
//              Message *msg
//                  Pointer to message.
//-----------------------------------------------------------------------------

void
NETWORK_ProcessEvent(Node *node, Message *msg)
{
   #ifdef ADDON_BOEINGFCS
    if ((msg->eventType == MSG_Receive_Only_Start)||
        (msg->eventType == MSG_Receive_Only_Stop )||
        (msg->eventType == MSG_Receive_Only_Black_To_Red_Stop)||
        (msg->eventType == MSG_Receive_Only_Black_To_Red_Start)
        )
    {
        ModeCesWnwReceiveOnlyLayer(node, msg);
        return;
    }
    #endif

    if (MESSAGE_GetProtocol(msg) == NETWORK_PROTOCOL_ARP)
    {
        ArpLayer(node, msg);
        return;
    }

    // Dual IP handling should be properly done here
    switch (node->networkData.networkProtocol)
    {
        case IPV4_ONLY:
        {
            NetworkIpLayer(node, msg);
            break;
        }
        case IPV6_ONLY:
        {
            Ipv6Layer(node, msg);
            break;
        }
        case DUAL_IP:
        {
            // DualIp enabled node
            switch (msg->protocolType)
            {
                case NETWORK_PROTOCOL_IPV6:
                case ROUTING_PROTOCOL_OSPFv3:
                case ROUTING_PROTOCOL_AODV6:
                case ROUTING_PROTOCOL_DYMO6:
                {
                    Ipv6Layer(node, msg);
                        break;
                }
                default:
                {
                    NetworkIpLayer(node, msg);
                    break;
                }
            }
            break;
        }
#ifdef CELLULAR_LIB
        case GSM_LAYER3:
        {
            GsmLayer3Layer(node, msg);
            break;
        }
#endif
#ifdef WIRELESS_LIB
        case CELLULAR:
        {
            if (MESSAGE_GetProtocol(msg) ==
                NETWORK_PROTOCOL_CELLULAR)
            {
                CellularLayer3Layer(node, msg);
            }
            else
            {
                // only IPv4 is supported for CELLULAR
                NetworkIpLayer(node, msg);
            }
        }
        break;
#endif // WIRELESS_LIB
        default:
            printf("%u\n", MESSAGE_GetProtocol(msg));
            ERROR_ReportError("Undefined switch value");
            break;
    }
}

//-----------------------------------------------------------------------------
// Finalize function
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FUNCTION     NETWORK_Finalize()
// PURPOSE      Calls finalize function of IP.
// PARAMETERS   Node *node
//                  Pointer to node.
//-----------------------------------------------------------------------------

void
NETWORK_Finalize(Node *node)
{
    ArpFinalize(node);

    switch (node->networkData.networkProtocol)
    {
        case IPV4_ONLY:
        {
            NetworkIpFinalize(node);
            break;
        }
        case IPV6_ONLY:
        {
            Ipv6Finalize(node);
            break;
        }
        case DUAL_IP:
        {
            Ipv6Finalize(node);
            NetworkIpFinalize(node);
            break;
        }
#ifdef CELLULAR_LIB
        case GSM_LAYER3:
        {
            GsmLayer3Finalize(node);
            MEM_free(node->networkData.gsmLayer3Var);
            NetworkIpFinalize(node);
            break;
        }
#endif
#ifdef WIRELESS_LIB
        case CELLULAR:
        {
            CellularLayer3Finalize(node);
            MEM_free(node->networkData.cellularLayer3Var);
            NetworkIpFinalize(node);
            break;
        }
#endif // WIRELESS_LIB
        default:
            ERROR_ReportError("Undefined switch value");
            break;
    }
#ifdef CYBER_LIB
#ifdef DO_ECC_CRYPTO
    if (caNeedInitialization == FALSE)
    {
        for (int i=0; i<12; i++)
        {
            mpi_free(caEccKey[i]);
        }
        caNeedInitialization = TRUE;
    }
#endif // DO_ECC_CRYPTO
#endif // CYBER_LIB
#ifdef CYBER_CORE
    if (node->networkData.pkiData != NULL)
    {
        PKIFinalize(node);
    }
#endif // CYBER_CORE
}


//-----------------------------------------------------------------------------
// FUNCTION     NETWORK_ReceivePacketFromMacLayer()
// PURPOSE      Network-layer receives packets from MAC layer, now check
//              type of IP and call proper function
// PARAMETERS   Node *node       : Pointer to node
//              Message *message : Message received
//              MacHWAddress* lastHopMacAddress : last hop mac address
//              int interfaceIndex : incoimg interface
//-----------------------------------------------------------------------------
void NETWORK_ReceivePacketFromMacLayer(Node* node,
                                       Message* msg,
                                       MacHWAddress* lastHopMacAddress,
                                       int interfaceIndex)
{
    IpHeaderType *ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
    //Not present in original code
    ip6_hdr *ipv6Header = (ip6_hdr *) MESSAGE_ReturnPacket(msg);
    NodeAddress lastHopAddress = ANY_DEST;

#ifdef WIRELESS_LIB
    if (node->networkData.networkProtocol == CELLULAR &&
        node->macData[interfaceIndex]->macProtocol == MAC_PROTOCOL_CELLULAR)
    {
        lastHopAddress = MacHWAddressToIpv4Address(node,
                                                   interfaceIndex,
                                                   lastHopMacAddress);

        CellularLayer3ReceivePacketFromMacLayer(
            node,
            msg,
            lastHopAddress,
            interfaceIndex);
        return;
    }
#endif
    if (node->networkData.networkProtocol != DUAL_IP)
    {
        if ((node->networkData.networkProtocol == IPV4_ONLY
             && IpHeaderGetVersion(ipHeader->ip_v_hl_tos_len) != IPVERSION4)
            || (node->networkData.networkProtocol == IPV6_ONLY
                && IpHeaderGetVersion(ipHeader->ip_v_hl_tos_len)
                != IPV6_VERSION))
        {
            // drop the packet due to IP version mismatch
            MESSAGE_Free(node, msg);
            return;
        }
    }
    // though V4 version is taken 3 bits instead of standard 4 bits,
    // V6 version use 4 bits as standard. Of course it will not make
    // problem in identifying the proper version.
    if (IpHeaderGetVersion(ipHeader->ip_v_hl_tos_len) == IPVERSION4)
    {
        if (NetworkIpGetInterfaceType(node, interfaceIndex) != NETWORK_IPV4
            && NetworkIpGetInterfaceType(node, interfaceIndex) != NETWORK_DUAL)
        {
            // drop the packet due to IP version mismatch
#ifdef ADDON_DB
        HandleNetworkDBEvents(
            node,
            msg,
            interfaceIndex,
            "NetworkPacketDrop",
            "IP Version Mismatch",
            0,
            0,
            0,
            0);
#endif
            MESSAGE_Free(node, msg);
            return;
        }

        lastHopAddress = MacHWAddressToIpv4Address(node,
                                                    interfaceIndex,
                                                    lastHopMacAddress);

        // started IPv4 processing of the packet
        NetworkIpReceivePacketFromMacLayer(node,
                                           msg,
                                           lastHopAddress,
                                           interfaceIndex);
    }
    else if (ip6_hdrGetVersion(ipv6Header->ipv6HdrVcf) ==
        IPV6_VERSION)
    {
        if (NetworkIpGetInterfaceType(node, interfaceIndex) != NETWORK_IPV6
          && NetworkIpGetInterfaceType(node, interfaceIndex) != NETWORK_DUAL)
        {
            // drop the packet due to IP version mismatch
            MESSAGE_Free(node, msg);
            return;
        }

        // started IPv6 processing of the packet
        Ipv6ReceivePacketFromMacLayer(node,
                                      msg,
                                      lastHopMacAddress,
                                      interfaceIndex);
    }
    else
    {
        char errMsg[MAX_STRING_LENGTH];
        sprintf(errMsg,
                "Node%d: Packet from MAC layer contains invalid version "
                "number %d in the IP header!",
                node->nodeId,
                IpHeaderGetVersion(ipHeader->ip_v_hl_tos_len));
        ERROR_ReportWarning(errMsg);
#ifdef ADDON_DB
        HandleNetworkDBEvents(
            node,
            msg,
            interfaceIndex,
            "NetworkPacketDrop",
            "IP Version Invalid",
            0,
            0,
            0,
            0);
#endif
        MESSAGE_Free(node, msg);
        return;
    }
}

//-----------------------------------------------------------------------------
// Statistics
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FUNCTION     NETWORK_RunTimeStat()
// PURPOSE      Calls run-time stats function of IP.
// PARAMETERS   Node *node
//                  Pointer to node.
//-----------------------------------------------------------------------------

void
NETWORK_RunTimeStat(Node *node)
{
    int i;
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;

    NetworkIpRunTimeStat(node);

    // Runtime Stat support for most of these protocols is incomplete.
    for (i = 0; i < node->numberInterfaces; i++)
    {
        switch (ip->interfaceInfo[i]->routingProtocolType)
        {
            case ROUTING_PROTOCOL_LAR1:
            {
                break;
            }
            case ROUTING_PROTOCOL_OSPFv2:
            {
                break;
            }
            case ROUTING_PROTOCOL_AODV:
            {
                break;
            }
            case ROUTING_PROTOCOL_DSR:
            {
                break;
            }
            case ROUTING_PROTOCOL_STAR:
            {
                break;
            }
            case MULTICAST_PROTOCOL_STATIC:
            {
                break;
            }
            case ROUTING_PROTOCOL_NONE:
            {
                break;
            }

#ifdef WIRELESS_LIB
            case ROUTING_PROTOCOL_ZRP:
            {
                ZrpRunTimeStat(node, (ZrpData *) ip->interfaceInfo[i]->routingProtocol);
                break;
            }
            case ROUTING_PROTOCOL_IARP:
            {
                IarpRunTimeStat(node, (IarpData *) ip->interfaceInfo[i]->routingProtocol);
                break;
            }
            case ROUTING_PROTOCOL_IERP:
            {
                IerpRunTimeStat(node, (IerpData *) ip->interfaceInfo[i]->routingProtocol);
                break;
            }
#endif // WIRELESS_LIB
//InsertPatch STATS_FUNCTION

            default:
                break;

        }//switch//
    }//for//
}


//-----------------------------------------------------------------------------
// FUNCTION     NetworkGetInterfaceInfo()
// PURPOSE      Returns interface information for a interface. Information
//              means its address and type
//
// PARAMETERS   Node *node       : Pointer to node
//              int interfaceIndex : interface index for which info required
//              Address* address : interface info returned
//-----------------------------------------------------------------------------
void NetworkGetInterfaceInfo(
    Node* node,
    int interfaceIndex,
    Address* address,
    NetworkType networkType)
{
    NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;

    if (networkType == NETWORK_IPV4
        && (ip->interfaceInfo[interfaceIndex]->interfaceType == NETWORK_IPV4
        || ip->interfaceInfo[interfaceIndex]->interfaceType == NETWORK_DUAL))
    {
        SetIPv4AddressInfo(
            address,
            ip->interfaceInfo[interfaceIndex]->ipAddress);
        return;
    }
    else if (networkType == NETWORK_IPV6
        && (ip->interfaceInfo[interfaceIndex]->interfaceType == NETWORK_IPV6
        || ip->interfaceInfo[interfaceIndex]->interfaceType == NETWORK_DUAL))
    {
        SetIPv6AddressInfo(
            address,
           ip->interfaceInfo[interfaceIndex]->ipv6InterfaceInfo->globalAddr);
        return;
    }

    address->networkType = NETWORK_INVALID;
}

//-----------------------------------------------------------------------------
// FUNCTION     NetworkIpGetInterfaceAddressString()
// PURPOSE      Returns address (IPv4 or V6) in the form of string
//
// PARAMETERS   Node *node       : Pointer to node
//              int interfaceIndex : relevant interface index
//              char ipAddrString[] : address string
//-----------------------------------------------------------------------------
void NetworkIpGetInterfaceAddressString(
    Node *node,
    const int interfaceIndex,
    char ipAddrString[])
{
    if (NetworkIpGetInterfaceType(node, interfaceIndex) == NETWORK_IPV4
        || NetworkIpGetInterfaceType(node, interfaceIndex) == NETWORK_DUAL)
    {
        NodeAddress interfaceAddress =
            NetworkIpGetInterfaceAddress(node, interfaceIndex);
        IO_ConvertIpAddressToString(interfaceAddress, ipAddrString);
    }
    else if (NetworkIpGetInterfaceType(node, interfaceIndex) == NETWORK_IPV6)
    {
        in6_addr interfaceAddress = {{{0}}};

        Ipv6GetGlobalAggrAddress(node, interfaceIndex, &interfaceAddress);

        IO_ConvertIpAddressToString(&interfaceAddress, ipAddrString);
    }
    else
    {
        ERROR_ReportError("Handle DualIP situation\n");
    }
}


//-----------------------------------------------------------------------------
// FUNCTION     NetworkIpGetInterfaceType()
// PURPOSE      Returns interface type of an interface
//
// PARAMETERS   Node *node       : Pointer to node
//              int interfaceIndex : relevant interface index
//-----------------------------------------------------------------------------
NetworkType NetworkIpGetInterfaceType(
    Node *node,
    int interfaceIndex)
{
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    if (ip->interfaceInfo[interfaceIndex]->interfaceType == NETWORK_IPV4)
    {
        // Ipv4 network
        return NETWORK_IPV4;
    }
    else if (ip->interfaceInfo[interfaceIndex]->interfaceType ==
        NETWORK_IPV6)
    {
        // Ipv6 network
        return NETWORK_IPV6;
    }
    else if (ip->interfaceInfo[interfaceIndex]->interfaceType ==
        NETWORK_DUAL)
    {
        // Ipv6 network
        return NETWORK_DUAL;
    }
    else
    {
        //Err:  Unknown network Type
        ERROR_ReportError("Unknown Network type Found.");
    }
    return NETWORK_IPV4;
}

// --------------------------------------------------------------------------
// FUNCTION: NETWORK_ManagementReport
// PURPOSE: Dispatches network management requests into Network layer
// --------------------------------------------------------------------------
void
NETWORK_ManagementReport(
    Node *node,
                            int interfaceIndex,
    ManagementReport* report,
    ManagementReportResponse* resp)
{

    assert(report != NULL && resp != NULL);

#ifdef ADDON_BOEINGFCS
    switch (node->networkData.networkVar->cesNetworkProtocol) {
        case NETWORK_PROTOCOL_CES_WNW_MI:

            NetworkWnwMiManagementReport(
                node,
                interfaceIndex,
                report,
                resp);

            break;
        default:
            resp->result = ManagementReportResponseUnsupported;
            resp->data = 0;
            break;
    }
#endif
}
