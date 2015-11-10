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
// This files contains initialization function, message processing
// function, and finalize function used for transport layer.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "qualnet_error.h"
#include "transport_udp.h"
#include "transport_tcp.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#ifdef ENTERPRISE_LIB
#include "transport_rsvp.h"
#endif // ENTERPRISE_LIB
#include "partition.h"

#include "network_ip.h"
#include "ipv6.h"
#include "transport_tcpip.h"
//InsertPatch HEADER_FILES

//-------------------------------------------------------------------------
// FUNCTION    TRANSPORT_Initialize
// PURPOSE     Initialization function for the TRANSPORT layer.
//
// Parameters:
//     node:      node being initialized.
//     nodeInput: structure containing contents of input file
//-------------------------------------------------------------------------
void TRANSPORT_Initialize(Node * node, const NodeInput * nodeInput)
{
    BOOL wasFound = FALSE;
    char buf[MAX_STRING_LENGTH];

    node->transportData.tcpType = TCP_REGULAR;
    node->transportData.tcp = NULL;
    node->transportData.udp = NULL;

    TransportTcpInit(node, nodeInput);
    TransportUdpInit(node, nodeInput);

    //
    // Initialize RSVP.
    //
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRANSPORT-PROTOCOL-RSVP",
        &wasFound,
        buf);

    if (!wasFound || strcmp(buf, "YES") == 0)
    {
        // Defaults to use RSVP when parameter not found.
        node->transportData.rsvpProtocol = TRUE;
        // RsvpInit(node, nodeInput);
    }
    else
    if (strcmp(buf, "NO") == 0)
    {
        node->transportData.rsvpProtocol = FALSE;
    }
    else
    {
        ERROR_ReportError("Expecting YES or NO for "
                          "TRANSPORT-PROTOCOL-RSVP parameter\n");
    }

    // InsertPatch TRANSPORT_INIT_CODE
}

//-------------------------------------------------------------------------
// FUNCTION    TRANSPORT_Finalize
// PURPOSE     Called at the end of simulation to collect the results of
//             the simulation of the TRANSPORT Layer.
//
// Parameter:
//     node:     node for which results are to be collected.
//-------------------------------------------------------------------------
void TRANSPORT_Finalize(Node * node)
{
    TransportUdpFinalize(node);
    TransportTcpFinalize(node);

#ifdef ENTERPRISE_LIB
    if (node->transportData.rsvpProtocol == TRUE)
    {
        RsvpFinalize(node);
    }
#endif // ENTERPRISE_LIB

    //InsertPatch FINALIZE_FUNCTION
}

//-------------------------------------------------------------------------
// FUNCTION    TRANSPORT_ProcessEvent
// PURPOSE     Models the behaviour of the TRANSPORT Layer on receiving the
//             message
//
// Parameters:
//     node:     node which received the message
//     msg:      message received by the layer
//-------------------------------------------------------------------------
void TRANSPORT_ProcessEvent(Node * node, Message * msg)
{
    switch (MESSAGE_GetProtocol(msg))
    {
        case TransportProtocol_UDP:
        {
            TransportUdpLayer(node, msg);
            break;
        }
        case TransportProtocol_TCP:
        {
            TransportTcpLayer(node, msg);
            break;
        }
        case TransportProtocol_RSVP:
        {
#ifdef ENTERPRISE_LIB
            if (node->transportData.rsvpProtocol == FALSE)
            {
                // MPLS module is not enabled on this node. Just ignore it.
                // Free the message.
                MESSAGE_Free(node, msg);
            }
            else
            {
                RsvpLayer(node, msg);
            }
            break;
#else // ENTERPRISE_LIB
            ERROR_ReportError("ENTERPRISE addon missing");
#endif // ENTERPRISE_LIB
        }
        //InsertPatch LAYER_FUNCTION
        default:
            assert(FALSE); abort();
            break;
    }//switch//
}


void
TRANSPORT_RunTimeStat(Node *node)
{

    //UDP RUNTIMESTAT FUNCTION SHOULD BE CALLED HERE
    //TCP RUNTIMESTAT FUNCTION SHOULD BE CALLED HERE

    if (node->transportData.rsvpProtocol == TRUE)
    {
        //Call RSVP Runtimestat function
    }

//InsertPatch TRANSPORT_STATS


}

//-----------------------------------------------------------------------------
// FUNCTION     TRANSPORT_Reset()
// PURPOSE      Reset and Re-initialize the Transport protocols and/or
//              transport layer.
//
// PARAMETERS   Node *node       : Pointer to node
//              nodeInput: structure containing contents of input file
//-----------------------------------------------------------------------------
void
TRANSPORT_Reset(Node *node, const NodeInput *nodeInput)
{
#ifdef ADDON_NGCNMS
    struct_transport_reset_function* function;
    function = node->transportData.resetFunctionList->first;

    //Function Destructor Ptrs
    while (function != NULL)
    {
        (function->FuncPtr)(node, nodeInput);
        function = function->next;
    }

    //nothing to free
    //TRANSPORT_DESTRUCTOR(node, nodeInput);

    TRANSPORT_Initialize(node, nodeInput); 
#endif
}

//-----------------------------------------------------------------------------
// FUNCTION     TRANSPORT_AddResetFunctionList()
// PURPOSE      Add which protocols in the transport layer to be reset to a
//              fuction list pointer.
//
// PARAMETERS   Node *node       : Pointer to node
//              voind *param: pointer to the protocols reset function
//-----------------------------------------------------------------------------
void 
TRANSPORT_AddResetFunctionList(Node* node, void *param)
{
#ifdef ADDON_NGCNMS
    struct_transport_reset_function* function;

    function = new struct_transport_reset_function;
    function->FuncPtr = (TransportSetFunctionPtr)param;
    function->next = NULL;

    if (node->transportData.resetFunctionList->first == NULL)
    {
        node->transportData.resetFunctionList->last = function;
        node->transportData.resetFunctionList->first = 
            node->transportData.resetFunctionList->last;
    }    
    else
    {
        node->transportData.resetFunctionList->last->next = function;
        node->transportData.resetFunctionList->last = 
            node->transportData.resetFunctionList->last->next;
    }
#endif
}

