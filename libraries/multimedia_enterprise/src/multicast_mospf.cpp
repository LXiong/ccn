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
// File   : mospf.c
// Purpose: To simulate MULTICAST OPEN SHORTEST PATH FIRST (MOSPF)
//          It is the multicast extension of the
//          unicast routing protocol Ospfv2.


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "api.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#include "qualnet_error.h"
#include "network_ip.h"
#include "routing_ospfv2.h"
#include "multicast_igmp.h"
#include "multicast_mospf.h"

#include "network_dualip.h"

#define DEBUG 0
#define DEBUG_ERROR 0


/////////////////////////////////////////////////////////////////////////
//                 Prototype declaration
/////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------//
//    Various Function To handle Group Related Information for Mospf
//----------------------------------------------------------------------//
static
void MospfOriginateGroupMembershipLSA(
    Node* node,
    unsigned int areaId,
    NodeAddress groupId,
    BOOL flush);

//----------------------------------------------------------------------//
//   Various Function To handle Shortest Path Calculation for Mospf
//----------------------------------------------------------------------//

static
void MospfCalculateShortestPath(
    Node* node,
    unsigned int areaId,
    MospfForwardingTableRow* newEntry);

//----------------------------------------------------------------------//
//   Various Function To handle Packet Forwarding Using Mospf
//----------------------------------------------------------------------//

static
void MospfModifyDownstreamlistOfForwardingTable(
    Node* node,
    MospfVertexItem* tempV,
    MospfForwardingTableRow* fwdTableRowPtr);

//----------------------------------------------------------------------//


//----------------------------------------------------------------------//
//   Various Trace Functions In Mospf
//----------------------------------------------------------------------//

// /**
// FUNCTION   :: MospfPrintTraceXMLLsa
// LAYER      :: NETWORK
// PURPOSE    :: Print MOSPF Specific Trace
// PARAMETERS ::
// + node      : Node* : Pointer to node, doing the packet trace
// + lsaHdr    : Ospfv2LinkStateHeader* : Pointer to Ospfv2LinkStateHeader
// RETURN     ::  void : NULL
// **/

void MospfPrintTraceXMLLsa(Node* node, Ospfv2LinkStateHeader* lsaHdr)
{
    char buf[MAX_STRING_LENGTH];
    MospfGroupMembershipInfo *grpInfo;
    int numGroupInfo;
    int i;
    char addr[20];

    numGroupInfo = (lsaHdr->length - sizeof(MospfGroupMembershipLSA))
                    / sizeof(MospfGroupMembershipInfo);
    if (numGroupInfo)
    {
        grpInfo = (MospfGroupMembershipInfo *)(lsaHdr + 1);

        for (i = 0; i < numGroupInfo; i++)
        {
            sprintf(buf, " <mospfLsaBody>");
            TRACE_WriteToBufferXML(node, buf);

            Ospfv2PrintTraceXMLLsaHeader(node, lsaHdr);

            IO_ConvertIpAddressToString(grpInfo[i].vertexID, addr);
            sprintf(buf, " %hu %s </mospfLsaBody>", grpInfo[i].vertexType, addr);
            TRACE_WriteToBufferXML(node, buf);
        }
    }
}


//----------------------------------------------------------------------//
//   Various Function To Print Different Table/List Used In Mospf
//----------------------------------------------------------------------//

//--------------------------------------------------------------------//
// NAME       : MospfPrintForwardingTable
// PURPOSE    : display all entries in forwarding table of the node.
// PARAMETER  : node
// RETURN     : None.
//--------------------------------------------------------------------//

static
void MospfPrintForwardingTable(Node* node)
{
    FILE* fp;
    unsigned int i;
    char clockTime[100];
    char pktSrcAddr[100];
    char groupAddr[100];
    char upStream[100];

    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);

    MospfForwardingTable* fwdTable = &mospf->forwardingTable;
    ctoa(node->getNodeTime(), clockTime);

    fp = fopen("FWDTable.txt", "a+");

    if (fp == NULL)
    {
        fprintf(stdout, "Couldn't open file FWDTable.txt\n");
        return;
    }

    fprintf (fp, "Forwarding Table for node %u has %d entry at %s\n",
        node->nodeId, fwdTable->numRows, clockTime);

    fprintf(fp, "\n  Group        Source     upStream   Num      "
        "I_Addr   I_Index \n");
    fprintf(fp, " ***********************************************"
        "****************\n");

    if (fwdTable->numRows != 0)
    {
        for (i = 0; i < fwdTable->numRows; i++)
        {
            ListItem* tempListItem;

            IO_ConvertIpAddressToString(fwdTable->row[i].groupAddr,
                groupAddr);
            IO_ConvertIpAddressToString(fwdTable->row[i].pktSrcAddr,
                pktSrcAddr);
            IO_ConvertIpAddressToString(fwdTable->row[i].upStream,
                upStream);

            fprintf (fp, "%10s %10s %10s    %3d \n",
                groupAddr, pktSrcAddr, upStream,
                fwdTable->row[i].downStream->size);

            tempListItem = fwdTable->row[i].downStream->first;

            while (tempListItem)
            {
                MospfdownStream* downStream =
                    (MospfdownStream*) tempListItem->data;
                char interfaceAddress[100];
                NodeAddress intfAddress = NetworkIpGetInterfaceAddress(
                                    node, downStream->interfaceIndex);

                IO_ConvertIpAddressToString(intfAddress, interfaceAddress);

                fprintf(fp, "   %50s    %3d\n",
                    interfaceAddress,
                    downStream->interfaceIndex);

                tempListItem = tempListItem->next;
            }//End while
        }
    }
    else
    {
        fprintf(fp, "    No Item In F_Table\n");
    }

    fprintf (fp, "\n\n");
    fclose(fp);
}

//--------------------------------------------------------------------//
// NAME        : MospfPrintThisList
// PURPOSE     : Print the content of the given list.
// RETURN      : None.
//--------------------------------------------------------------------//

static
void MospfPrintThisList(
    Node* node,
    LinkedList* myList)
{
    ListItem* item;
    char vertexId[MAX_STRING_LENGTH];

    MospfVertexItem* entry;

    printf(" The list :\n");

    printf("\nType    Id    cost  IL  LSA  Ass.Int   P_type   P_Id\n");
    printf("------------------------------------------------------\n");


    item = myList->first;

    while (item)
    {
        entry = (MospfVertexItem*) item->data;

        IO_ConvertIpAddressToString(entry->vId, vertexId);

        printf("%2d   %6s  %3d   %d   %d  %10x ",
            entry->vType, vertexId,
            entry->cost,
            entry->incomingLinkType,
            entry->LSA->linkStateType,
            entry->associatedIntf);

        if (entry->parent != NULL)
        {
            printf("   %2d %8x \n",
                entry->parent->vType, entry->parent->vId);
        }
        else
        {
            printf("\n");
        }

        item = item->next;
    }

    printf("\n");
}


//--------------------------------------------------------------------//
// NAME     : MospfPrintGroupMembershipLSAListForThisArea()
// PURPOSE  : Print the content of the GroupMembership
//          : LSA list for this area.
// RETURN   : None.
//--------------------------------------------------------------------//

static
void MospfPrintGroupMembershipLSAListForThisArea(Node* node,
                                            Ospfv2Area* thisArea)
{
    Ospfv2ListItem* item;
    char clockStr[MAX_CLOCK_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

    printf("    GroupMembership LSA list for node %u, area 0x%x at %s\n",
        node->nodeId, thisArea->areaID, clockStr);
    printf("    size is %d\n", thisArea->groupMembershipLSAList->size);

    item = thisArea->groupMembershipLSAList->first;

    // Print each link state information that we have in our
    // link state database.

    while (item)
    {
        Ospfv2PrintLSA((char*) item->data);

        item = item->next;
    }
}


//--------------------------------------------------------------------//
// NAME     : MospfPrintGroupMembershipLSAList
// PURPOSE  : Print the content of the groupMembership LSA list.
// RETURN   : None.
//--------------------------------------------------------------------//

static
void MospfPrintGroupMembershipLSAList(Node* node)
{
    Ospfv2ListItem* item;

    Ospfv2Data* ospf = (Ospfv2Data*)NetworkIpGetRoutingProtocol(node,
        ROUTING_PROTOCOL_OSPFv2);

    for (item = ospf->area->first; item; item = item->next)
    {
        Ospfv2Area* thisArea = (Ospfv2Area*) item->data;
        MospfPrintGroupMembershipLSAListForThisArea(node, thisArea);
    }
}

//----------------------------------------------------------------------//
//          Various Initialization Function for Mospf
//----------------------------------------------------------------------//


//--------------------------------------------------------------------//
// NAME       : MospfInitInterface
// PURPOSE    : Initialize the available network interface.
// RETURN     : None.
// ASSUMPTION : At present Forwarding type is not kept
//              as configurationable parameter, hence initialy
//              we keep it as disabled .Otherwise it needs to be set
//              according to the specified value in config file.
//--------------------------------------------------------------------//

static
void MospfInitInterface(Node* node)
{
    int i;
    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);

    mospf->mInterface = (MospfInterface*)
        MEM_malloc(sizeof(MospfInterface) *
        node->numberInterfaces);

    for (i = 0; i < node->numberInterfaces; i++)
    {
        mospf->mInterface[i].forwardingType = DATA_LINK_DISABLED;
        mospf->mInterface[i].linkType
            = (Ospfv2LinkType) MOSPF_INVALID_LINKTYPE;
    }   //end of for;
}


//--------------------------------------------------------------------//
// NAME         :MospfInitStat()
// PURPOSE      :Initializes statistics structure for Mospf
// ASSUMPTION   :None
// RETURN VALUE :NULL
//--------------------------------------------------------------------//

static
void MospfInitStat(Node* node)
{
    MospfData* mospf = (MospfData*) NetworkIpGetMulticastRoutingProtocol(
                                        node,
                                        MULTICAST_PROTOCOL_MOSPF);

    memset(&mospf->stats, 0, sizeof(MospfStats));

#ifdef ADDON_DB
    mospf->mospfSummaryStats.m_NumGroupLSAGenerated = 0;
    mospf->mospfSummaryStats.m_NumGroupLSAFlushed = 0;
    mospf->mospfSummaryStats.m_NumGroupLSAReceived = 0;
    mospf->mospfSummaryStats.m_NumRouterLSA_WCMRSent = 0;
    mospf->mospfSummaryStats.m_NumRouterLSA_WCMRReceived = 0;
    mospf->mospfSummaryStats.m_NumRouterLSA_VLEPSent = 0;
    mospf->mospfSummaryStats.m_NumRouterLSA_VLEPReceived = 0;
    mospf->mospfSummaryStats.m_NumRouterLSA_ASBRSent = 0;
    mospf->mospfSummaryStats.m_NumRouterLSA_ASBRReceived = 0;
    mospf->mospfSummaryStats.m_NumRouterLSA_ABRSent = 0;
    mospf->mospfSummaryStats.m_NumRouterLSA_ABRReceived = 0;

    strcpy(mospf->mospfMulticastNetSummaryStats.m_ProtocolType,"");
    mospf->mospfMulticastNetSummaryStats.m_NumDataSent = 0;
    mospf->mospfMulticastNetSummaryStats.m_NumDataRecvd = 0;
    mospf->mospfMulticastNetSummaryStats.m_NumDataForwarded = 0;
    mospf->mospfMulticastNetSummaryStats.m_NumDataDiscarded = 0;
#endif
}


//--------------------------------------------------------------------//
// NAME         :MospfInitForwardingTable()
// PURPOSE      :Initializes forwarding table of Mospf
// ASSUMPTION   :None
// RETURN VALUE :NULL
//--------------------------------------------------------------------//

static
void MospfInitForwardingTable(Node* node)
{
    MospfData* mospf = (MospfData*)
       NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);

    MospfForwardingTable* forwardingTable = &mospf->forwardingTable;

    int size = sizeof(MospfForwardingTableRow) * OSPFv2_INITIAL_TABLE_SIZE;

    forwardingTable->numRows = 0;
    forwardingTable->allottedNumRow = OSPFv2_INITIAL_TABLE_SIZE;
    forwardingTable->row = (MospfForwardingTableRow*) MEM_malloc(size);

    memset(forwardingTable->row, 0, size);
}


//--------------------------------------------------------------------//
// NAME         : MospfCheckForInterAreaMulticastFwder()
// PURPOSE      : Check if this node Is InterAreaMulticastFwder
// RETURN VALUE : NULL
//--------------------------------------------------------------------//

static
void MospfCheckForInterAreaMulticastFwder(
    Node* node,
    const NodeInput* nodeInput)
{
    char currentLine[MAX_STRING_LENGTH];
    // For IO_GetDelimitedToken
    char iotoken[MAX_STRING_LENGTH];
    char* next;
    const char* delims = "{,} \t\n";
    char* token;
    char* ptr;
    unsigned int id;
    int i;
    unsigned int* idPtr;

    MospfData* mospf = (MospfData*) NetworkIpGetMulticastRoutingProtocol
        (node, MULTICAST_PROTOCOL_MOSPF);

    for (i = 0; i < nodeInput->numLines; i++)
    {
        if (strcmp(nodeInput->variableNames[i],
                "INTER-AREA-MULTICAST-FORWARDER") == 0)
        {
            if ((ptr = strchr(nodeInput->values[i], '{')) == NULL)
            {
                char errStr[MAX_STRING_LENGTH];

                sprintf(errStr, "Could not find '{' character:\n  %s\n",
                    nodeInput->values[i]);
                ERROR_ReportError(errStr);
            }
            else
            {
                strcpy(currentLine, ptr);
                token = IO_GetDelimitedToken(
                            iotoken, currentLine, delims, &next);

                if (!token)
                {
                    char errStr[MAX_STRING_LENGTH];

                    sprintf(errStr, "Cann't find nodeId list, e.g. "
                                    "{1, 2, ..}:\n%s\n",
                        currentLine);
                    ERROR_ReportError(errStr);
                }

                while (TRUE)
                {
                    id = (unsigned int) atoi(token);
                    idPtr = &id;

                    if (id == node->nodeId)
                    {
                        if (DEBUG_ERROR)
                        {
                            printf("Node %u can be I_A_M_F\n",
                                node->nodeId);
                        }

                        mospf->interAreaMulticastFwder = TRUE;
                    }

                    // Retrieve next token.
                    token = IO_GetDelimitedToken(
                                iotoken, next, delims, &next);

                    if (!token)
                    {
                        break;
                    }
                    if (strncmp("thru", token, 4) == 0)
                    {
                        unsigned int maxNodeId;
                        unsigned int minNodeId;

                        minNodeId = id;

                        // Retrieve next token.
                        token = IO_GetDelimitedToken(
                                    iotoken, next, delims, &next);
                        if (!token)
                        {
                            char errStr[MAX_STRING_LENGTH];

                            sprintf(errStr, "Bad string input\n%s",
                                currentLine);
                            ERROR_ReportError(errStr);
                        }
                        maxNodeId = atoi(token);
                        if (maxNodeId < minNodeId)
                        {
                            char errStr[MAX_STRING_LENGTH];

                            sprintf(errStr, "Bad string input\n%s",
                                currentLine);
                            ERROR_ReportError(errStr);
                        }
                        if ((node->nodeId >= minNodeId)
                            && (node->nodeId <= maxNodeId))
                        {
                            if (DEBUG_ERROR)
                            {
                                printf("Node %u can be I-A-M-F\n",
                                    node->nodeId);
                            }

                            mospf->interAreaMulticastFwder = TRUE;
                        }
                        else
                        {
                            // Retrieve next token.
                            token = IO_GetDelimitedToken(
                                        iotoken, next, delims, &next);
                        }
                        if (!token)
                        {
                            break;
                        }
                    }
                } //End while;
            } //End else;
        } // End of if;
    }//End for;
}


//--------------------------------------------------------------------//
// NAME       : MospfSetInterfaceLinkTypeAndForwardingType
// PURPOSE    : Set the LinkType And ForwardingType of
//              the available network interface.
// RETURN     : None.
// ASSUMPTION : We assume that forwarding type is not a user's
//              configurable parameter. In future if it is given
//              as input in configuration file it needs to be
//              set from Initi function
//--------------------------------------------------------------------//

static
void MospfSetInterfaceLinkTypeAndForwardingType(
    Node* node,
    int interfaceIndex)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node,
                                             MULTICAST_PROTOCOL_MOSPF);

    if ((mospf->mInterface[interfaceIndex].forwardingType
            != DATA_LINK_DISABLED)
        && (mospf->mInterface[interfaceIndex].linkType
                != MOSPF_INVALID_LINKTYPE))
    {
        // link type & forwarding type is already set;
        if (DEBUG_ERROR)
        {
            printf("For Node %u & interface %d \n",
                node->nodeId, interfaceIndex);
            printf(" link type & forwarding type is already set\n");
        }
        return;
    }

    // check for stub Network
    if (ospf->iface[interfaceIndex].neighborList->size != 0)
    {
        if (ospf->iface[interfaceIndex].type
            == OSPFv2_BROADCAST_INTERFACE)
        {
            //Send packet out this interface as data link multicast
            mospf->mInterface[interfaceIndex].forwardingType
                = DATA_LINK_MULTICAST;
            mospf->mInterface[interfaceIndex].linkType
                = OSPFv2_TRANSIT;
        }
        else if (ospf->iface[interfaceIndex].type
            == OSPFv2_POINT_TO_POINT_INTERFACE ||
            ospf->iface[interfaceIndex].type
            == OSPFv2_POINT_TO_MULTIPOINT_INTERFACE)
        {
            //Send packet out this interface as data link unicast
            mospf->mInterface[interfaceIndex].forwardingType
                = DATA_LINK_MULTICAST;
            mospf->mInterface[interfaceIndex].linkType
                = OSPFv2_POINT_TO_POINT;
        }
    }   //if not stub
    else
    {
        //Send packet out this interface as data link unicast
        mospf->mInterface[interfaceIndex].forwardingType
            = DATA_LINK_MULTICAST;
        mospf->mInterface[interfaceIndex].linkType
            = OSPFv2_STUB;
    }
}


//----------------------------------------------------------------------//
//    Various Function To handle Group Related Information for Mospf
//----------------------------------------------------------------------//

//--------------------------------------------------------------------//
// NAME         :MospfScheduleGroupMembershipLSA()
// PURPOSE      :Schedule group Membership LSA origination
// ASSUMPTION   :None
// RETURN VALUE :None
//--------------------------------------------------------------------//

void MospfScheduleGroupMembershipLSA(
    Node* node,
    unsigned int areaId,
    unsigned int interfaceId,
    NodeAddress groupId,
    BOOL flush)
{
    Message* newMsg;
    clocktype delay;
    char* msgInfo;
    Ospfv2LinkStateType lsType;
    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);

    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);

    if (groupId == (unsigned) ANY_GROUP)
    {
        LinkedList* grpList;

        // Collect group info from Igmp database
        IgmpFillGroupList(node, &grpList, interfaceId);

        if ((!grpList->size) && (!flush))
        {
            if (DEBUG_ERROR)
            {
                printf("Node %u :IGMP inform about %d group\n",
                    node->nodeId, grpList->size);
            }
            ListFree(node, grpList, FALSE);
            return;
        }

        ListFree(node, grpList, FALSE);
    }

    if ((thisArea->groupMembershipLSAOriginateTime == 0)
        || ((node->getNodeTime() - thisArea->groupMembershipLSAOriginateTime)
                >= OSPFv2_MIN_LS_INTERVAL))
    {
        delay = (clocktype) (RANDOM_erand(mospf->seed)
                                * OSPFv2_BROADCAST_JITTER);
    }
    else
    {
        delay = (clocktype) (OSPFv2_MIN_LS_INTERVAL
                            - (node->getNodeTime()
                            - thisArea->groupMembershipLSAOriginateTime));
    }

    thisArea->groupMembershipLSTimer = TRUE;

    newMsg = MESSAGE_Alloc(
                node,
                NETWORK_LAYER,
                ROUTING_PROTOCOL_OSPFv2,
                MSG_ROUTING_OspfScheduleLSDB);

    MESSAGE_InfoAlloc(
        node, newMsg,
        sizeof(Ospfv2LinkStateType) + sizeof(unsigned int)
            + sizeof(BOOL) + sizeof(int) + sizeof(NodeAddress));

    lsType = OSPFv2_GROUP_MEMBERSHIP;
    msgInfo = MESSAGE_ReturnInfo(newMsg);

    memcpy(msgInfo, &lsType, sizeof(Ospfv2LinkStateType));
    msgInfo += sizeof(Ospfv2LinkStateType);
    memcpy(msgInfo, &areaId, sizeof(unsigned int));
    msgInfo += sizeof(unsigned int);
    memcpy(msgInfo, &flush, sizeof(BOOL));
    msgInfo += sizeof(BOOL);
    memcpy(msgInfo, &interfaceId, sizeof(int));
    msgInfo += sizeof(int);
    memcpy(msgInfo, &groupId, sizeof(NodeAddress));

    MESSAGE_Send(node, newMsg, (clocktype) delay);
}


//--------------------------------------------------------------------//
// NAME      : MospfVertexPresentInList
// PURPOSE   : check if the vertex is already included in the list
// RETURN    : TRUE if present in list otherwise FALSE
//--------------------------------------------------------------------//

static
BOOL MospfVertexPresentInList(
    MospfGroupMembershipInfo* linkList,
    int count)
{
    int index;

    for (index = 0; index < count; index++)
    {
        if ((linkList[index].vertexType == linkList[count].vertexType)
            && (linkList[index].vertexID == linkList[index].vertexID))
        {
            return TRUE;
        }
    }
    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME         :MospfInstallGroupMembershipLSAInLSDB()
// PURPOSE      :Installing LSAs in the database & generate LSA in
//               BackboneArea for corresponding group of non backBone Area
// RETURN       :TRUE if LSA is installed properly, FALSE otherwise.
//-------------------------------------------------------------------------//

BOOL MospfInstallGroupMembershipLSAInLSDB(
    Node* node,
    unsigned int areaId,
    char* LSA)
{
    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);

    BOOL retVal = FALSE;
    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);

    if (DEBUG_ERROR)
    {
        printf("Node %u has GrpMbrList Size %d for area 0x%x before\n",
            node->nodeId, thisArea->groupMembershipLSAList->size,
            areaId);
    }

    retVal = Ospfv2InstallLSAInLSDB(node,
                                    thisArea->groupMembershipLSAList,
                                    LSA,
                                    thisArea->areaID);
    if (retVal)
    {
        Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;

        if ((mospf->interAreaMulticastFwder == TRUE)
            && (areaId != OSPFv2_BACKBONE_AREA))
        {
            // From RFC 1584 sec 3.1:
            // In order to convey group membership information between areas,
            // inter-area multicast forwarders "summarize" their attached
            // areas group membership information to the backbone

            if (DEBUG)
            {
                char grpAddrStr[MAX_STRING_LENGTH];
                char areaAddrStr[MAX_STRING_LENGTH];

                IO_ConvertIpAddressToString(LSHeader->linkStateID,
                                            grpAddrStr);
                IO_ConvertIpAddressToString(areaId, areaAddrStr);

                printf("Node %u (IAMF) received an group membership LSA "
                    "for group %s in area %s\n"
                    "Needs to originate group membership LSA for Backbone\n",
                    node->nodeId, grpAddrStr, areaAddrStr);
            }

            MospfScheduleGroupMembershipLSA(
                node,
                (unsigned int) OSPFv2_BACKBONE_AREA,
                (unsigned int) ANY_INTERFACE,
                LSHeader->linkStateID,
                FALSE);
        }
    }
    return retVal;
}


//-------------------------------------------------------------------------//
// NAME         :MospfCheckForGrpMbrInNonBackBoneArea()
//
// PURPOSE      :This function will check whether membership for a particular
//               group exist in any of the attached non-backbone area. The
//               function is called from MospfOriginateGroupMembershipLSA()
//               in case the originating router is 'Inter Area Multicast
//               Forwarder' and the area for which LSA is originating is
//               backbone area.
//
// RETURN       :TRUE if this group is present in any of the attached
//               Non BackBone area, FALSE otherwise.
//-------------------------------------------------------------------------//

static
BOOL MospfCheckForGrpMbrInNonBackBoneArea(
    Node* node,
    NodeAddress groupId)
{
    Ospfv2Data* ospf = (Ospfv2Data*)NetworkIpGetRoutingProtocol(node,
        ROUTING_PROTOCOL_OSPFv2);
    int i;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        // At least one of this Router's attached non-backbone
        // areas has Group G members then Router should add itself
        // to the advertisement

        if (ospf->iface[i].areaId != OSPFv2_BACKBONE_AREA)
        {
            Ospfv2Area* thisArea = Ospfv2GetArea(
                                        node, ospf->iface[i].areaId);

            Ospfv2LinkStateHeader* listLSHeader;
            Ospfv2ListItem* item;

            item = thisArea->groupMembershipLSAList->first;
            while (item)
            {
                // Get LS Header
                listLSHeader = (Ospfv2LinkStateHeader*) item->data;

                if (DEBUG_ERROR)
                {
                    printf(" listLSHeader->advertisingRouter %u\n",
                        listLSHeader->advertisingRouter);
                    printf(" listLSHeader->linkStateID %u\n",
                        listLSHeader->linkStateID);
                }

                // indicated by the presence of one or more advertisements
                // in the areas' link state databases having Link State ID
                // set to Group G and LS age is not MaxAge
                if ((listLSHeader->linkStateID == groupId)        
                    && (Ospfv2MaskDoNotAge(ospf, listLSHeader->linkStateAge) != 
                           OSPFv2_LSA_MAX_AGE / SECOND))
                {
                    return TRUE;
                }

                item = item->next;
            } // check each group present in the area;
        } // end of if;
    } // for all interface;
    return FALSE;
}

//--------------------------------------------------------------------//
// NAME        : MospfCheckGroupMembershipLSABody()
// PURPOSE     : Compare old & new LSA
// ASSUMPTION  : TOS Field remains unaltered
// RETURN VALUE: TRUE  if LSA changed
//--------------------------------------------------------------------//

BOOL MospfCheckGroupMembershipLSABody(
    MospfGroupMembershipLSA* newLSA,
    MospfGroupMembershipLSA* oldLSA)
{
    int i, j;
    BOOL* sameLinkInfo;
    int oldNumLinks = (oldLSA->LSHeader.length
                        -  (sizeof(MospfGroupMembershipLSA)))
                        / (sizeof(MospfGroupMembershipInfo));

    int newNumLinks = (newLSA->LSHeader.length
                        -  (sizeof(MospfGroupMembershipLSA)))
                        / (sizeof(MospfGroupMembershipInfo));

    MospfGroupMembershipInfo* newLinkList;
    MospfGroupMembershipInfo* oldLinkList;

    newLinkList = (MospfGroupMembershipInfo*) (newLSA + 1);
    oldLinkList = (MospfGroupMembershipInfo*) (oldLSA + 1);

    if (newNumLinks != oldNumLinks)
    {
        return TRUE;
    }

    // Make all flags False
    sameLinkInfo = (BOOL*) MEM_malloc(sizeof(BOOL) * oldNumLinks);
    memset(sameLinkInfo, 0, (sizeof(BOOL) * oldNumLinks));

    for (i = 0; i < newNumLinks; i++)
    {
        for (j = 0; j < oldNumLinks; j++)
        {
            if (sameLinkInfo[j] != TRUE)
            {
                if ((newLinkList->vertexID == oldLinkList->vertexID)
                    && (newLinkList->vertexType == oldLinkList->vertexType))
                {
                    sameLinkInfo[j] = TRUE;
                    break;
                }   //id
            }   //already matched

            oldLinkList
                = (MospfGroupMembershipInfo*)(oldLinkList + 1);

        }   // end of for;

        newLinkList = (MospfGroupMembershipInfo*)(newLinkList + 1);
    }   //end of for;

    // check if any link info in old LSA changed
    for (j = 0; j < oldNumLinks; j++)
    {
        if (sameLinkInfo[j] != TRUE)
        {
            MEM_free(sameLinkInfo);
            return TRUE;
        }
    }

    MEM_free(sameLinkInfo);
    return FALSE;
}


//-------------------------------------------------------------------------//
// NAME         :MospfFlushLSAForThisGroup()
// PURPOSE      :Flush LSA from this area and if it is an IAMF
//               convey this information for back bone area
// ASSUMPTION   :None
// RETURN VALUE :None
//-------------------------------------------------------------------------//

static
void MospfFlushLSAForThisGroup(
    Node* node,
    char* LSA,
    unsigned int areaId)
{
    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    Ospfv2LinkStateHeader* LSHeader = (Ospfv2LinkStateHeader*) LSA;
    NodeAddress groupId;

    if (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) ==
           (OSPFv2_LSA_MAX_AGE/SECOND))
    {
        return;
    }

    LSHeader->linkStateAge = (OSPFv2_LSA_MAX_AGE / SECOND);
    groupId = LSHeader->linkStateID ;

    // Flood LSA and and remove from own LSDB
    Ospfv2FloodLSA(node, ANY_INTERFACE, (char*) LSA, ANY_DEST, areaId);

    Ospfv2AddLsaToMaxAgeLsaList(node, LSA, areaId);

    if ((mospf->interAreaMulticastFwder == TRUE)
        && (areaId != OSPFv2_BACKBONE_AREA))
    {
        // From RFC 1584 sec 3.1:
        // In order to convey group membership information between areas,
        // inter-area multicast forwarders "summarize" their attached
        // areas group membership information to the backbone

        if (DEBUG)
        {
            char grpAddrStr[MAX_STRING_LENGTH];
            char areaAddrStr[MAX_STRING_LENGTH];

            IO_ConvertIpAddressToString(groupId, grpAddrStr);
            IO_ConvertIpAddressToString(areaId, areaAddrStr);

            printf("Node %u (IAMF): Needs to inform backbone about "
                "expiration of group %s in area %s\n",
                node->nodeId, grpAddrStr, areaAddrStr);
        }

        MospfScheduleGroupMembershipLSA(
            node,
            (unsigned int) OSPFv2_BACKBONE_AREA,
            (unsigned int) ANY_INTERFACE,
            groupId,
            TRUE);
    }

    mospf->stats.numGroupMembershipLSAFlushed++;
#ifdef ADDON_DB
    mospf->mospfSummaryStats.m_NumGroupLSAFlushed++;
#endif
}


//--------------------------------------------------------------------//
// NAME    : MospfOriginateGroupMembershipLSA()
//
// PURPOSE : Each group-membership-LSA concerns a single multicast group.
//           Essentially, the group-membership-LSA lists those vertices which
//           are directly connected to the LSA's originator and which contain
//           one or more group members.
//
// RETURN  : None.
//--------------------------------------------------------------------//

static
void MospfOriginateGroupMembershipLSA(
    Node* node,
    unsigned int areaId,
    NodeAddress groupId,
    BOOL flush)
{
    int i;
    unsigned int count = 0;

    Ospfv2LinkStateHeader* oldLSHeader;
    Ospfv2ListItem* listItem;
    Ospfv2Interface* thisInterface;
    MospfGroupMembershipInfo* linkList;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);

    int listSize = node->numberInterfaces;
    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);

    linkList = (MospfGroupMembershipInfo*)
                    MEM_malloc(sizeof(MospfGroupMembershipInfo) * listSize);

    memset(linkList, 0, sizeof(MospfGroupMembershipInfo) * listSize);

    listItem = thisArea->connectedInterface->first;

    while (listItem)
    {
        BOOL localMember = FALSE;
        thisInterface = (Ospfv2Interface*) listItem->data;
        i = thisInterface->interfaceIndex;

        // If interface state is down, no link should be added
        if (ospf->iface[i].state == S_Down)
        {
            listItem = listItem->next;
            continue;
        }

        IgmpSearchLocalGroupDatabase(node, groupId, i, &localMember);

        if (localMember == TRUE)
        {
            // set the link type & forwarding type for this interface
            MospfSetInterfaceLinkTypeAndForwardingType(node, i);

            // set the body of the Group Membership LSA

            // if not stub network check the interface link type
            if ((mospf->mInterface[i].linkType == OSPFv2_TRANSIT)
                && (ospf->iface[i].address
                == ospf->iface[i].designatedRouterIPAddress))
            {
                linkList[count].vertexType =
                    MOSPF_VERTEX_TRANSIT_NETWORK;
                linkList[count].vertexID = ospf->iface[i].address;

                if (!MospfVertexPresentInList(linkList, count))
                {
                    count++;
                }
            }
            // if it is a stub or point to point network;
            else
            {
                linkList[count].vertexType = MOSPF_VERTEX_ROUTER;
                linkList[count].vertexID = ospf->routerID;

                if (!MospfVertexPresentInList(linkList, count))
                {
                    count++;
                }
            }
        }   //if local member;

        listItem = listItem->next;
    }   //for all interface;

    if ((mospf->interAreaMulticastFwder == TRUE)
        && (areaId == OSPFv2_BACKBONE_AREA))
    {
        // RFC 1584:
        // Area A is the backbone area and at least one of this
        // Router's attached non-backbone areas has Group G members
        // (indicated by the presence of one or more advertisements
        // in the areas' link state databases having Link State ID
        // set to Group G and LS age set to a value other than MaxAge),
        // then Router should add itself to the advertisement by adding
        // a vertex with Vertex type set to 1 (router) and Vertex ID
        // set to Router's OSPF Router ID.

        if (MospfCheckForGrpMbrInNonBackBoneArea(node, groupId))
        {
            linkList[count].vertexType = MOSPF_VERTEX_ROUTER;
            linkList[count].vertexID = ospf->routerID;

            if (!MospfVertexPresentInList(linkList, count))
            {
                count++;
            }
        } // if one or more group member is present;
    } // if inter area multicast forwarder;

    // Get old instance, if any..
    oldLSHeader = Ospfv2LookupLSAList(
                        thisArea->groupMembershipLSAList,
                        ospf->routerID, groupId);

    if ((!count) && (oldLSHeader != NULL))
    {
        // We need to flush this old LSA as currently no
        // member for this group is associated with this node
        if (DEBUG)
        {
            printf("Node %u flush LSA\n", node->nodeId);
            Ospfv2PrintLSHeader(oldLSHeader);
        }

        MospfFlushLSAForThisGroup(node, (char*) oldLSHeader, areaId);
    }
    else
    {
        MospfGroupMembershipLSA* LSA;

        // Otherwise start constructing the GroupMembership LSA
        LSA = (MospfGroupMembershipLSA*)
                    MEM_malloc(sizeof(MospfGroupMembershipLSA)
                        + (sizeof(MospfGroupMembershipInfo) * count));
        memset(LSA, 0, sizeof(MospfGroupMembershipLSA)
                        + (sizeof(MospfGroupMembershipInfo) * count));

        if (oldLSHeader == NULL)
        {
            if (DEBUG_ERROR)
            {
                printf("Node %u has no old entry\n", node->nodeId);
            }
        }

        if (!Ospfv2CreateLSHeader(
                node, areaId, OSPFv2_GROUP_MEMBERSHIP,
                &LSA->LSHeader, oldLSHeader))
        {
            MEM_free(linkList);
            MEM_free(LSA);
            MospfScheduleGroupMembershipLSA(
                node,
                areaId,
                (unsigned int) ANY_INTERFACE,
                groupId,
                FALSE);
            return;
        }

        LSA->LSHeader.linkStateID = groupId;
        LSA->LSHeader.length = (short)(sizeof(MospfGroupMembershipLSA)
                                + (sizeof(MospfGroupMembershipInfo)
                                    * count));

        if (DEBUG_ERROR)
        {
            printf("Node %u LS_Hdr has: \n", node->nodeId);
            printf(" LSA->LSHeader.linkStateID %u\n",
                LSA->LSHeader.linkStateID);
            printf(" LSA->LSHeader.linkStateSequenceNumber %d \n",
                LSA->LSHeader.linkStateSequenceNumber);
            printf(" LSA->LSHeader.multicast %d \n",
                Ospfv2OptionsGetMulticast(LSA->LSHeader.options));
            printf(" LSA->LSHeader.linkStateAge %d \n",
                LSA->LSHeader.linkStateAge);
        }

        // Copy my link information into the LSA.
        memcpy(
            LSA + 1, linkList,
            ((sizeof(MospfGroupMembershipInfo) * count)));

        if (MospfInstallGroupMembershipLSAInLSDB(node, areaId, (char*) LSA))
        {
            if (DEBUG)
            {
                printf("now adding this LSA to own LSDB in Area 0x%x\n",
                    areaId);
            }
        }

        if (DEBUG)
        {
            printf("Node %u Flood this new self_originated LSA"
                " in area0x%x\n", node->nodeId, areaId);
            Ospfv2PrintLSHeader(&(LSA->LSHeader));
        }

        // Flood LSA
        Ospfv2FloodLSA(node, ANY_INTERFACE,(char*) LSA, ANY_DEST, areaId);

        mospf->stats.numGroupMembershipLSAGenerated++;
#ifdef ADDON_DB
        mospf->mospfSummaryStats.m_NumGroupLSAGenerated++;
#endif
        MEM_free(LSA);
    }   //if one or more transit vertex are present;

    // Note LSA Origination time
    thisArea->groupMembershipLSAOriginateTime = node->getNodeTime();
    thisArea->groupMembershipLSTimer = FALSE;

    // Modify Shortest Path and Fwd Table
    MospfModifyShortestPathAndForwardingTable(node);

    if (DEBUG)
    {
        MospfPrintGroupMembershipLSAListForThisArea(node, thisArea);
    }

    MEM_free(linkList);
}


//-------------------------------------------------------------------------//
// NAME        : MospfCheckLocalGroupDatabaseToOriginateGroupMembershipLSA()
//
// PURPOSE     : Check Local Group Database and determine for which groups
//               we need to originate LSA. If groupId parameter is ANY_GROUP
//               originate LSA for each group in local database. Otherwise
//               originate LSA for a specific group.
//
// PARAMETERS  : 1) Pointer to node.
//               2) areaId for which we are generating LSA.
//               3) groupId (ANY_GROUP or a specific one)
//               4) flush specific group information or not.
//
// RETURN      : NULL
//-------------------------------------------------------------------------//

void MospfCheckLocalGroupDatabaseToOriginateGroupMembershipLSA(
    Node* node,
    unsigned int areaId,
    unsigned int interfaceId,
    NodeAddress groupId,
    BOOL flush)
{
    LinkedList* grpList;
    ListItem* tempItem;

    if (groupId != (unsigned) ANY_GROUP)
    {
        if (DEBUG_ERROR)
        {
            printf("Node %u checking to update grm_LSA for",
                node->nodeId);
            printf("associated group %u & area 0x%x\n",
                groupId, areaId);
        }

        MospfOriginateGroupMembershipLSA(node, areaId, groupId, flush);
    }
    else
    {
        // Collect local group information from IGMP
        IgmpFillGroupList(node, &grpList, interfaceId);

        if (DEBUG_ERROR)
        {
            printf("Node %u :IGMP inform about %d group\n",
                node->nodeId, grpList->size);
        }

        // Originate LSA for each group in local group database.
        tempItem = grpList->first;

        while (tempItem)
        {
            memcpy(&groupId, tempItem->data, sizeof(NodeAddress));

            if (DEBUG_ERROR)
            {
                printf("Node %u checking to update grm_LSA \n",
                    node->nodeId);
                printf("associated group %u & area 0x%x\n",
                    groupId, areaId);
            }

            MospfOriginateGroupMembershipLSA(node, areaId, groupId, flush);

            tempItem = tempItem->next;
        }

        ListFree(node, grpList, FALSE);
    }
}


//--------------------------------------------------------------------//
// NAME         : MospfSearchLSDBToFindMatchingRouterLSA()
// PURPOSE      : Search the Router LSA For from the Link State dataBase
//                to check if it a wild card Multicast Receiver
// RETURN VALUE : Ospfv2LinkStateHeader* ;
//--------------------------------------------------------------------//

static
Ospfv2LinkStateHeader* MospfSearchLSDBToFindMatchingRouterLSA(
    Node* node,
    NodeAddress sourceAddr,
    unsigned int  areaId)
{
    Ospfv2LinkStateHeader* listLSHeader;
    Ospfv2ListItem* tempItem;
    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    if (DEBUG_ERROR)
    {
        printf("Node %u Looking for LS_org from router LSAList of 0x%x\n",
          node->nodeId, areaId);
        printf(" this area has %d LSA in LSA list\n",
          thisArea->routerLSAList->size);
    }

    // TBD: We consider every node is ospf router.
    // In future to include normal
    // hosts as source we need to search Network LSAs also.

    // Looking for vertex desired LSA
    tempItem = thisArea->routerLSAList->first;

    while (tempItem)
    {
        // Get LS Header
        listLSHeader = (Ospfv2LinkStateHeader*) tempItem->data;

        if (DEBUG_ERROR)
        {
            printf("     Searching for Link State Origin %u\n",
                sourceAddr);
            printf("  available AdvertisingRouter %u\n",
                listLSHeader->advertisingRouter);
        }

        // Skip LSA if max age is reached and examine the next LSA.
        if (Ospfv2MaskDoNotAge(ospf, listLSHeader->linkStateAge) >=
               (OSPFv2_LSA_MAX_AGE / SECOND))
        {
            if (DEBUG)
            {
                printf(" Node %u : max age so discard LSA of Adrouter %u\n",
                    node->nodeId, listLSHeader->advertisingRouter);
            }

            tempItem = tempItem->next;
            continue;
        }

        if (listLSHeader->advertisingRouter == sourceAddr)
        {
            if (DEBUG_ERROR)
            {
                printf("LSAHdr FOUND!\n");
                printf("   age = %d\n", listLSHeader->linkStateAge);
                printf("   M_cast = %d \n",
                    Ospfv2OptionsGetMulticast(listLSHeader->options));
                printf("   Type = %d\n", listLSHeader->linkStateType);
                printf("   Ad_router = %u\n",
                  listLSHeader->advertisingRouter);
                printf("   LinkStateId = %u\n", listLSHeader->linkStateID);
            }

            return listLSHeader;
        }

        tempItem = tempItem->next;
    }   //end of while;

    return NULL;
}


//--------------------------------------------------------------------//
// NAME         : MospfUpdateShortestPathListForThisArea()
// PURPOSE      : Find next cadidate which is closest to source
// RETURN VALUE : MospfVertexItem*
//--------------------------------------------------------------------//

static
MospfVertexItem* MospfUpdateShortestPathListForThisArea(
    Node* node,
    LinkedList* cList,
    LinkedList* sList)
{
    ListItem* tempItem = NULL;
    ListItem* item = NULL;
    int lowestMetric = MOSPF_LS_INFINITY;

    MospfVertexItem* vInfo = (MospfVertexItem*)
                                    MEM_malloc(sizeof(MospfVertexItem));
    memset(vInfo, 0, sizeof(MospfVertexItem));

    tempItem = cList->first;
    item = tempItem;

    if (DEBUG_ERROR)
    {
        printf("  Node %u has candidate list of size %d\n",
            node->nodeId, cList->size);
    }

    // Get the vertex with the smallest metric from the candidate List
    while (tempItem)
    {
        MospfVertexItem* tempVertex = (MospfVertexItem*) tempItem->data;

        if (tempVertex->cost < lowestMetric)
        {
            lowestMetric = tempVertex->cost;
            item = tempItem;
        }
        else if (tempVertex->cost == lowestMetric)
        {
            // If there are multiple possibilities select
            // transit networks over routers.
            MospfVertexItem* prevVertex = (MospfVertexItem*) item->data;

            if ((tempVertex->vType == MOSPF_VERTEX_TRANSIT_NETWORK)
                && (prevVertex->vType == MOSPF_VERTEX_ROUTER))
            {
                lowestMetric = tempVertex->cost;
                item = tempItem;
            }

            // If there are still multiple possibilities remaining,
            //  select the vertex having the highest Vertex ID.
            else if (tempVertex->vId > prevVertex->vId)
            {
                lowestMetric = tempVertex->cost;
                item = tempItem;
            }
        }

        tempItem = tempItem->next;
    } //for all vertex;

    memcpy(vInfo, item->data, sizeof(MospfVertexItem));

    if (DEBUG_ERROR)
    {
        printf("  the vertex added in Shortest path is %x\n",
            vInfo->vId);
        printf("    size of candidateList now %d\n", cList->size);
    }
    // Add to Shortest path List
    ListInsert(node, sList, 0, (void*) vInfo);

    // And remove it from the candidate list since no longer available
    ListGet(node, cList, item, TRUE, FALSE);

    return vInfo;
}



//--------------------------------------------------------------------//
// NAME         : MospfVertexItemLabelledWithGroup();
// PURPOSE      : check if the vertex is labelled with desired group
// RETURN VALUE : TRUE if labelled with group
//--------------------------------------------------------------------//

static
BOOL MospfVertexItemLabelledWithGroup(
    Node* node,
    unsigned int areaId,
    MospfVertexItem* tempV,
    NodeAddress groupAddr)

{
    Ospfv2ListItem* tempItem;
    NodeAddress originator;
    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);
    Ospfv2Data* ospf = (Ospfv2Data*)NetworkIpGetRoutingProtocol(node,
        ROUTING_PROTOCOL_OSPFv2);

    if (DEBUG_ERROR)
    {
        printf("Node %u search for \n", node->nodeId);
        printf("    Vertex Id = %x\n", tempV->vId);
        printf("    Vertex type = %d\n", tempV->vType);
        printf(" the size of grmlist %d\n",
            thisArea->groupMembershipLSAList->size);
        MospfPrintGroupMembershipLSAList(node);
    }

    originator = tempV->LSA->advertisingRouter;

    // check this area's group membership List
    tempItem = thisArea->groupMembershipLSAList->first;

    // Looking for vertex
    while (tempItem)
    {
      MospfGroupMembershipLSA* LSAInfo;
      LSAInfo = (MospfGroupMembershipLSA*) tempItem->data;

      // Skip LSA if max age is reached and examine the next LSA.
      if (Ospfv2MaskDoNotAge(ospf, LSAInfo->LSHeader.linkStateAge) >=
             (OSPFv2_LSA_MAX_AGE / SECOND))
      {
          if (DEBUG)
          {
              printf(" size of GRMLSA %d \n",
                  thisArea->groupMembershipLSAList->size);
              printf(" checking for vertex %u\n", tempV->vId);

              printf("Node %u :max age so discard LSA of AdRtr %u\n",
                  node->nodeId, LSAInfo->LSHeader.advertisingRouter);
          }

          tempItem = tempItem->next;
          continue;
      }

      if (DEBUG_ERROR)
      {
          printf("  searching for vertex %u from Grm_LSAList \n",
              originator);
          printf("    available Ad_Router = %u\n",
              LSAInfo->LSHeader.advertisingRouter);
      }

      if ((LSAInfo->LSHeader.linkStateID == groupAddr)
          && (LSAInfo->LSHeader.advertisingRouter == originator))
      {
          if (DEBUG_ERROR)
          {
              printf("Node %u inform this vertex %u is labelled\n",
                  node->nodeId, tempV->vId);

              printf("   age = %d\n", LSAInfo->LSHeader.linkStateAge);
              printf("   M_cast = %d \n",
                  Ospfv2OptionsGetMulticast(LSAInfo->LSHeader.options));
              printf("   Type = %d\n", LSAInfo->LSHeader.linkStateType);
              printf("   Ad_router = %u\n",
                  LSAInfo->LSHeader.advertisingRouter);
              printf("   LinkStateId = %u\n",
                  LSAInfo->LSHeader.linkStateID);
          }
          return TRUE;
      }
      tempItem = tempItem->next;
    }   //end of while;

    // Vertex is not labelled with desired group, so return FALSE
    return FALSE;
}


//--------------------------------------------------------------------//
// NAME        : MospfIsMyAddress
// PURPOSE     : Determine if an IP address is my own.
// RETURN      : TRUE if it's my IP address, FALSE otherwise.
//--------------------------------------------------------------------//

static
BOOL MospfIsMyAddress(Node* node, NodeAddress addr)
{
    int i;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (addr == NetworkIpGetInterfaceAddress(node, i))
        {
            return TRUE;
        }
    }

    return FALSE;
}


//--------------------------------------------------------------------//
// NAME         : MospfModifyUpstreamNode()
// PURPOSE      : Update forwarding cache entry's upstream node
//                from associated shortest path tree
// RETURN VALUE : NULL
//--------------------------------------------------------------------//
static
void MospfModifyUpstreamNode(
    Node* node,
    LinkedList* shortestPathList,
    unsigned int areaId,
    MospfForwardingTableRow* fwdCacheRow)
{
    ListItem* item;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    // RFC 1584: 12.2.7
    // After the datagram shortest-path tree for Area A is complete,
    // the calculating router (RTX) must decide whether Area A,
    // out of all of its attached areas, determines the
    // forwarding cache entry's upstream node. This is done by
    // examining RTX's position on the Area A datagram SP tree,
    // which is in turn described by RTX's Area A Vertex data structure.

    // Search through the shortest path list for this node's routerId.

    item = shortestPathList->first;
    while (item)
    {
        MospfVertexItem* entry;
        entry = (MospfVertexItem*) item->data;

        // Examine RTX's position on the Area A datagram SP tree,
        // which is in turn described by it's Area A Vertex data structure.

        if ((ospf->routerID == entry->vId)
            && (entry->vType == MOSPF_VERTEX_ROUTER))

        {
            // If RTX's Vertex parameter IncomingLinkType
            // is either ILNone (RTX is not on the tree),
            // ILVirtual or ILSummary, then some area other
            // than Area A will determine the upstream node.

            if ((entry->incomingLinkType == MOSPF_IL_None)
                || (entry->incomingLinkType == MOSPF_IL_Virtual)
                || (entry->incomingLinkType == MOSPF_IL_Summary))
            {
                // then some area other than Area A will determine
                // the upstream node.
            }
            //Otherwise, Area A might possibly determine the
            // upstream node i.e., may be selected the RootArea
            else
            {
                // If RootArea has not been set,set RootArea to Area A.
                if (fwdCacheRow->rootArea == OSPFv2_INVALID_AREA_ID)
                {
                    fwdCacheRow->rootArea = areaId;
                    fwdCacheRow->rootCost = entry->cost;
                }
                // Otherwise, compare the present RootArea to Area A
                // if required update it
                else
                {
                    BOOL updateRootArea = FALSE;

                    // Choose the area that is "nearest to the source".
                    // Nearest to the source depends on each area's
                    // candidate list initialization case.

                    MospfVertexItem* firstEntry
                        = (MospfVertexItem*)(shortestPathList->first->data);

                    if (firstEntry->incomingLinkType == MOSPF_IL_Direct)
                    {
                        // this case is SourceIntraArea candidateList Init
                        // set RootArea to this
                        updateRootArea = TRUE;
                    }
                    else
                    {
                        //this case is SourceInterArea1 candidateList Init
                        // If this is the backboneset, set RootArea to this

                            if (areaId == OSPFv2_BACKBONE_AREA)
                            {
                                updateRootArea = TRUE;
                            }
                            else
                            {
                                // set RootArea to the area whose datagram
                                // SP tree provides the shortest path from
                                // SourceNet to RTX.
                                // This is a comparison of RTX's Vertex
                                // parameter Cost in the two areas.

                                if (entry->cost < fwdCacheRow->rootCost)
                                {
                                    updateRootArea = TRUE;
                                }
                                else if (entry->cost
                                    == fwdCacheRow->rootCost)
                                {
                                    // set RootArea to one with the highest
                                    // OSPF Area ID.
                                    if (areaId > fwdCacheRow->rootArea)
                                    {
                                        updateRootArea = TRUE;
                                    }
                                }   //check cost

                            }   //check for non backbone Area

                    }    //check for various inter-Area Case

                    if (updateRootArea)
                    {
                        fwdCacheRow->rootArea = areaId;
                        fwdCacheRow->rootCost = entry->cost;
                    }

                }   // check for modification in root area

            }   //select root area

            //if this is the RootArea, the forwarding cache entry's
            // upstream node must be set accordingly
            if (fwdCacheRow->rootArea == areaId)
            {

                // If IncomingLinkType is equal to ILDirect,
                // the upstream node is set to the appropriate
                // directly-connected stub network.

                if (entry->incomingLinkType == MOSPF_IL_Direct)
                {
                    // get the associated interface
                    // for this directly connected vertex

                    int interfaceId = NetworkGetInterfaceIndexForDestAddress
                                             (node, fwdCacheRow->pktSrcAddr);

                    fwdCacheRow->upStream =
                        NetworkIpGetInterfaceNetworkAddress(node, interfaceId);
                }
                // If equal to ILNormal, the upstream node
                // is set to the Parent field
                else if (entry->incomingLinkType == MOSPF_IL_Normal)
                {
                    fwdCacheRow->upStream = entry->parent->vId;
                }
                // If equal to ILExternal, the upstream node is set to
                // the placeholder EXTERNAL.
                else
                {
                    // TBD;
                    printf(" MOSPF_IL_External: not yet implemented \n");
                }

                fwdCacheRow->incomingIntf =
                    NetworkGetInterfaceIndexForDestAddress(node,
                    fwdCacheRow->upStream);

                if (DEBUG)
                {
                    printf("Node %u :Upstream modified For vertex %x\n",
                        node->nodeId, entry->vId);
                    printf("    upstream is %x\n", fwdCacheRow->upStream);
                    printf("    interface is %d\n",
                        fwdCacheRow->incomingIntf);
                }
            }  //if I am the root area;
            break;
        }  //vertex found;
        item = item->next;
    }  //end of while;
}


//--------------------------------------------------------------------//
// NAME         : MospfInShortestPathList()
// PURPOSE      : Check if the vertex is already included
//                in the shortest path tree.
// RETURN VALUE : TRUE if the vertex exists in shortest path
//--------------------------------------------------------------------//

static
BOOL MospfInShortestPathList(
    LinkedList* shortestPathList,
    NodeAddress vId,
    Ospfv2LinkType type)
{
    ListItem* item;
    MospfVertexType vType;

    if (type == OSPFv2_POINT_TO_POINT)
    {
        vType = MOSPF_VERTEX_ROUTER;
    }
    else if (type == OSPFv2_TRANSIT)
    {
        vType = MOSPF_VERTEX_TRANSIT_NETWORK;
    }
    else if (type == OSPFv2_STUB)
    {
        vType = MOSPF_VERTEX_ROUTER;
    }
    else
    {
        vType = MOSPF_VERTEX_ROUTER;
        ERROR_Assert(FALSE, "Unknown vertex type\n");
    }

    if (DEBUG_ERROR)
    {
        printf("    MospfShortestPathList has %d item\n",
            shortestPathList->size);
    }

    // Search through the shortest path list for the node.
    item = shortestPathList->first;
    while (item)
    {
        MospfVertexItem* entry;
        entry = (MospfVertexItem*) item->data;

        if (DEBUG_ERROR)
        {
            printf("In SPList->looking for vertex %x \n", vId);
            printf("          available vertex %x\n", entry->vId);
        }

        // Found it.
        if ((entry->vId == vId) && (entry->vType == vType))
        {
            if (DEBUG_ERROR)
            {
                printf("vertex %x is already in shortest path list\n",
                    vId);
            }

            return TRUE;
        }

        item = item->next;
    }   // end of while;

    if (DEBUG_ERROR)
    {
        printf("vertex %u is not in shortest path list\n", vId);
    }

    return FALSE;
}


//--------------------------------------------------------------------//
// NAME         : MospfSetLSAFieldForThisVertex()
// PURPOSE      : Set LSA depending on matching vType & vId from LSDB.
// RETURN       : None.
//--------------------------------------------------------------------//

static
void MospfSetLSAFieldForThisVertex(
    Node* node,
    unsigned int areaId,
    MospfVertexItem* v)
{
    Ospfv2ListItem* tempItem;
    Ospfv2LinkStateHeader* LSAInfo = NULL;
    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);
    v->LSA = NULL;

    if (v->vType == MOSPF_VERTEX_TRANSIT_NETWORK)
    {
        // search network LSA for matching entry
        tempItem = thisArea->networkLSAList->first;

        // Looking for vertex desired LSA
        while (tempItem)
        {
            LSAInfo
                = (Ospfv2LinkStateHeader*) tempItem->data;

            if (LSAInfo->linkStateID == v->vId)
            {
                v->LSA = LSAInfo;
                return;
            }

            tempItem = tempItem->next;
        }
    }

    else if (v->vType == MOSPF_VERTEX_ROUTER)
    {
        // search router LSA for matching entry
        tempItem = thisArea->routerLSAList->first;

        // Looking for vertex desired LSA
        while (tempItem)
        {
            LSAInfo =
                (Ospfv2LinkStateHeader*) tempItem->data;

            if (LSAInfo->linkStateID == v->vId)
            {
                v->LSA = LSAInfo;
                return;
            }

            tempItem = tempItem->next;
        }
    }

    else
    {
        ERROR_Assert(FALSE, "wrong vertex Type \n");
    }
}


//--------------------------------------------------------------------//
// NAME         : MospfSetIncomingLinkTypeForThisVertex()
// PURPOSE      : Set incoming link type depending on the
//                position of Srcnet.
// RETURN       : None.
//--------------------------------------------------------------------//
static
void MospfSetIncomingLinkTypeForThisVertex (
                                            Node* node,
                                            LinkedList* shortestPathList,
                                            MospfVertexItem* v)
{
    MospfVertexItem*  tempVertex;
    NodeAddress rootId;
    MospfIncomingLinkType rootILType;
    NodeAddress vNet;
    NodeAddress rootNet;
    Ospfv2RoutingTableRow* rowPtr;

    v->incomingLinkType = MOSPF_IL_None;

    tempVertex = (MospfVertexItem*)(shortestPathList->first->data);

    rootId = tempVertex->vId ;
    rootILType = tempVertex->incomingLinkType;

    // find the entry for root in Ospfv2 routing Table
    rowPtr = Ospfv2GetRouteToSrcNet(
            node, rootId, OSPFv2_DESTINATION_NETWORK);

    if (!rowPtr)
    {
        // check if I am the root
        if (MospfIsMyAddress (node, rootId))
        {
            // this vertex is received from my router's LSA
            // hence it is directly attached with me i.e. root
            v->incomingLinkType = MOSPF_IL_Direct;
            return;
        }
        else
        {
            return;
        }
    }

    // if root of the SP and srcNet of Pkt are directly connected
    if (rootILType == MOSPF_IL_Direct)
    {
        rootNet = MaskIpAddress(rootId, rowPtr->addrMask);

        vNet = MaskIpAddress(v->vId, rowPtr->addrMask);

        // if they are in same network
        if (rootNet == vNet)
        {
            v->incomingLinkType = MOSPF_IL_Direct;
        }
        else
        {
            v->incomingLinkType = MOSPF_IL_Normal;
        }

    }
    // root and srcNet are not directly connected
    // rootNet sumarize the srcNet entry in this area
    else if (rootILType == MOSPF_IL_Summary)
    {
        v->incomingLinkType = MOSPF_IL_Normal;
    }
}


//--------------------------------------------------------------------//
// NAME         : MospfUpdateCandidateListUsingRouterLSA()
// PURPOSE      : Update the candidate list using only the router LSAs.
// RETURN       : None.
//--------------------------------------------------------------------//

static
void MospfUpdateCandidateListUsingRouterLSA(
                                            Node* node,
                                            unsigned int areaId,
                                            LinkedList* shortestPathList,
                                            LinkedList* candidateList,
                                            MospfVertexItem* v)
{
    int i;
    Ospfv2RouterLSA* listLSA;
    Ospfv2LinkInfo* linkList;

    listLSA = (Ospfv2RouterLSA*)v->LSA ;

    linkList = (Ospfv2LinkInfo*) (listLSA + 1);

    if (DEBUG_ERROR)
    {
        printf(" BEFORE Updating the candidate list is \n");
        MospfPrintThisList(node, candidateList);
    }

    if (DEBUG_ERROR)
    {
        printf("        FOUND it!\n");
        printf("        now examining for %d links\n",
            listLSA->numLinks);
    }

    // Found vertex v.  Now examine its links.
    for (i = 0; i < listLSA->numLinks; i++)
    {
        if (DEBUG_ERROR)
        {
            printf("  examining link %x\n", linkList[i].linkID);
            printf("  having Link type %d\n", linkList[i].type);
        }

        // If LSA does not exist or does not have a link back to
        // vertex v, examine next link in v's LSA.
        if (((linkList[i].type == OSPFv2_POINT_TO_POINT) ||
            (linkList[i].type == OSPFv2_TRANSIT)) &&
            (!MospfInShortestPathList(
            shortestPathList, linkList[i].linkID,
            (Ospfv2LinkType) linkList[i].type)))
        {
            ListItem* tempItem;
            BOOL found = FALSE;

            MospfVertexItem* candidateListItem =
                (MospfVertexItem*)
                MEM_malloc(sizeof(MospfVertexItem));

            memset(candidateListItem, 0, sizeof(MospfVertexItem));

            if (DEBUG_ERROR)
            {
                printf(" time to set vertex Type\n");
            }

            // Select Vertex type
            if (linkList[i].type == OSPFv2_TRANSIT)
            {
                candidateListItem->vType =
                    MOSPF_VERTEX_TRANSIT_NETWORK;
            }
            else
            {
                candidateListItem->vType
                    = MOSPF_VERTEX_ROUTER;
            }

            // Set Vertex ID
            candidateListItem->vId = linkList[i].linkID;

            // SetIncomingLinkType
            MospfSetIncomingLinkTypeForThisVertex (
                node,
                shortestPathList,
                candidateListItem);

            if (candidateListItem->incomingLinkType == MOSPF_IL_None)
            {
                MEM_free(candidateListItem);
                continue;
            }

            // Set LSA field
            MospfSetLSAFieldForThisVertex(
                node, areaId, candidateListItem);

            if (candidateListItem->LSA == NULL)
            {
                MEM_free(candidateListItem);
                continue;
            }

            candidateListItem->cost =
                (char)(v->cost + (char) linkList[i].metric);

            // Set parent
           candidateListItem->parent = v;

            // Set AssociatedInterface

            // calculating-router is itself the vertex
            if (MospfIsMyAddress(
                node, listLSA->LSHeader.advertisingRouter))
            {
                candidateListItem->associatedIntf =
                    Ospfv2GetInterfaceIndexFromlinkData(
                                            node, linkList[i].linkData);
                candidateListItem->ttl = 1;
            }

            // Vertex V is upstream of the calculating router
            else if (v->associatedIntf == (unsigned) MOSPF_INVALID_INTERFACE)
            {
                candidateListItem->associatedIntf =
                    (unsigned int) MOSPF_INVALID_INTERFACE;
                candidateListItem->ttl = 0;
            }
            // Vertex is transit network and directly downstream
            // from the calculating router
            else if ((v->vType == MOSPF_VERTEX_TRANSIT_NETWORK)
                && (v->associatedIntf != (unsigned) MOSPF_INVALID_INTERFACE)
                && (v->ttl == 1))
            {
                candidateListItem->associatedIntf =
                    NetworkGetInterfaceIndexForDestAddress(node, v->vId);
                candidateListItem->ttl = 1;
            }
            // Vertex is downstream from calculating router
            else if ((v->associatedIntf != (unsigned) MOSPF_INVALID_INTERFACE)
                && ((v->ttl > 1)
                || (v->vType == MOSPF_VERTEX_ROUTER)))
            {
                candidateListItem->associatedIntf =
                    v->associatedIntf;
                candidateListItem->ttl = v->ttl;
            }
            else
            {
                ERROR_Assert(FALSE, "New vertex's location is not "
                    "determined\n");
            }

            if (DEBUG_ERROR)
            {
                printf("        LSA from %u\n",
                    listLSA->LSHeader.advertisingRouter);
                printf("linkID %u\n", linkList[i].linkID);
                printf("linkData %u\n", linkList[i].linkData);
            }

            tempItem = candidateList->first;

            // Update candidate List //
            while (tempItem)
            {
                MospfVertexItem* tempEntry;
                found = FALSE;

                tempEntry = (MospfVertexItem*) tempItem->data;


                // If candidate already in candidate list
                if (tempEntry->vId == candidateListItem->vId)
                {
                    BOOL updateReqd = FALSE;
                    found = TRUE;

                    // Update candidate in candidate list if new
                    // candidate has smaller metric.

                    if (tempEntry->cost > candidateListItem->cost)
                    {
                        updateReqd = TRUE;
                    }

                    // check for multiple posibilities
                    // If costs are equal, then the following
                    // tiebreakers are invoked.
                    else if (tempEntry->cost
                        == candidateListItem->cost)
                    {
                        // The type of Link is compared to W's current
                        // IncomingLinkType, and whichever link has
                        // the preferred type is chosen

                        if (tempEntry->incomingLinkType
                            > candidateListItem->incomingLinkType)
                        {
                            updateReqd = TRUE;
                        }

                        // If the link types are the same,
                        else if (tempEntry->incomingLinkType
                            == candidateListItem->incomingLinkType)
                        {
                            // then a link whose Parent is a transit
                            // network is preferred over one whose Parent
                            // is a router.

                            if (tempEntry->parent->vType
                                < candidateListItem->parent->vType)
                            {
                                updateReqd = TRUE;
                            }

                            // If the links are still equivalent,
                            else if (tempEntry->parent->vType
                                == candidateListItem->parent->vType)
                            {
                                //the link whose Parent has the higher
                                // Vertex ID is chosen.

                                if (tempEntry->parent->vId
                                    < candidateListItem->parent->vId)
                                {
                                    updateReqd = TRUE;

                                }   //check for parent Id

                            }   //check for parent Type

                        }   //check for incoming link

                    }   //check for cost

                    // check existing entry needs to be updated
                    if (updateReqd == TRUE)
                    {
                        tempEntry->vType = candidateListItem->vType ;
                        tempEntry->cost = candidateListItem->cost;
                        tempEntry->parent = candidateListItem->parent;

                        tempEntry->associatedIntf =
                            candidateListItem->associatedIntf;

                        tempEntry->LSA = candidateListItem->LSA;
                        tempEntry->ttl = candidateListItem->ttl;

                        tempEntry->incomingLinkType =
                            candidateListItem->incomingLinkType;


                        if (DEBUG_ERROR)
                        {
                            printf(" Node %u updating c_list for\n",
                                node->nodeId);
                            printf("   tempEntry->vId = %x\n",
                                tempEntry->vId);
                            printf("    tempEntry->parentId = %u\n",
                                tempEntry->parent->vId);
                            printf("    tempEntry->metric = %d\n",
                                tempEntry->cost);
                            printf("    tempEntry->ILType = %d\n",
                                tempEntry->incomingLinkType);
                            printf("    updated vertex %x with "
                                "metric %d and parent %u to "
                                "candidate list\n",
                                tempEntry->vId,
                                tempEntry->cost,
                                tempEntry->parent->vId);
                        }
                    }
                    break;
                }

                tempItem = tempItem->next;
            }

            if (!found)
            {
                if (DEBUG_ERROR)
                {
                    printf("adding vertex %u with parent "
                        "%u and metric %u to candidate list\n",
                        candidateListItem->vId,
                        candidateListItem->parent->vId,
                        candidateListItem->cost);
                }

                // New candidate, so add to candidate list.
                ListInsert(
                    node, candidateList, 0,
                    (void*) candidateListItem);
            }
            else
            {
                // No need to add to candidate list since
                // already there.

                if (DEBUG_ERROR)
                {
                    printf("No need to add to candidate list"
                        "since already there \n");
                }
                MEM_free(candidateListItem);
            }
        }
    }

    if (DEBUG_ERROR)
    {
        printf(" AFTER updating the candidate list is\n");
        printf("  Size of modified candidate list is %d\n",
            candidateList->size);
        MospfPrintThisList(node, candidateList);
    }
}


//--------------------------------------------------------------------//
// NAME     : MospfUpdateCandidateListUsingNetworkLSA()
// PURPOSE  : Update the candidate list using only the network LSAs.
// RETURN   : None.
//--------------------------------------------------------------------//

static
void MospfUpdateCandidateListUsingNetworkLSA(
    Node* node,
    unsigned int areaId,
    LinkedList* shortestPathList,
    LinkedList* candidateList,
    MospfVertexItem* v)
{
    int i;
    int numLink;
    Ospfv2NetworkLSA* listLSA;
    NodeAddress* attachedRtr;
    NodeAddress netMask;
    NodeAddress networkIPAddress;

    listLSA = (Ospfv2NetworkLSA*)v->LSA;

    attachedRtr = ((NodeAddress*) (listLSA + 1)) + 1;

    numLink = (listLSA->LSHeader.length
        - sizeof(Ospfv2NetworkLSA) - 4)
        / (sizeof(NodeAddress));

    memcpy(&netMask, listLSA + 1, sizeof(NodeAddress));

    if (DEBUG_ERROR)
    {
        printf("BEFORE updating the candidate list is\n");
        MospfPrintThisList(node, candidateList);
    }

    for (i = 0; i < numLink; i++)
    {
        if (DEBUG_ERROR)
        {
            printf("        examining VId %x\n", attachedRtr[i]);
            printf("        examining vType %d\n", OSPFv2_POINT_TO_POINT);
        }

        networkIPAddress = attachedRtr[i] & netMask;

        // If LSA does not exist or does not have a link back to
        // vertex v, examine next link in v's LSA.

        if (!MospfInShortestPathList(
            shortestPathList, attachedRtr[i],
            OSPFv2_POINT_TO_POINT))
        {
            ListItem* tempItem;
            BOOL found = FALSE;

            MospfVertexItem* candidateListItem =
                (MospfVertexItem*)
                MEM_malloc(sizeof(MospfVertexItem));

            memset(candidateListItem, 0, sizeof(MospfVertexItem));

            // Network LSA describes all the routers attached to
            // the network. So the vertex type for the new candidate
            // will always be router

            candidateListItem->vType = MOSPF_VERTEX_ROUTER;

            // Set vertex ID
            candidateListItem->vId = attachedRtr[i];

            // Set parent
            candidateListItem->parent = v;

            // SetIncomingLinkType
            MospfSetIncomingLinkTypeForThisVertex (
                node,
                shortestPathList,
                candidateListItem);

            if (candidateListItem->incomingLinkType == MOSPF_IL_None)
            {
                MEM_free(candidateListItem);
                continue;
            }

            // Set LSA field
            MospfSetLSAFieldForThisVertex(
                node, areaId, candidateListItem);

            if (candidateListItem->LSA == NULL)
            {
                MEM_free(candidateListItem);
                continue;
            }

            // The distance from a broadcast network to all
            // attached routers is 0.  Thus, next hop to destination
            // may not be the shortest path since transit networks
            // (ethernets) are treated as a pseudo-node.

            candidateListItem->cost = v->cost;

            // Set AssociatedInterface

            // Vertex is upstream of calculating router
            if (v->associatedIntf == (unsigned) MOSPF_INVALID_INTERFACE)
            {
                candidateListItem->associatedIntf
                    = (unsigned int) MOSPF_INVALID_INTERFACE;
                candidateListItem->ttl = 0;
            }

            // Vertex is transit network and directly downstream
            // from the calculating router

            else if ((v->vType == MOSPF_VERTEX_TRANSIT_NETWORK)
                && (v->associatedIntf != (unsigned) MOSPF_INVALID_INTERFACE)
                && (v->ttl == 1))
            {
                candidateListItem->associatedIntf =
                    NetworkGetInterfaceIndexForDestAddress(node, v->vId);
                candidateListItem->ttl = 1;
            }

            // Vertex is downstream from calculating router
            else if ((v->associatedIntf != (unsigned) MOSPF_INVALID_INTERFACE)
                && ((v->ttl > 1)
                || (v->vType == MOSPF_VERTEX_ROUTER)))
            {
                candidateListItem->associatedIntf =
                    v->associatedIntf;
                candidateListItem->ttl = v->ttl;
            }
            else
            {
                ERROR_Assert(FALSE, "New vertex's location is not "
                    "determined\n");
            }

            if (DEBUG_ERROR)
            {
                printf("        LSA from %u\n",
                    listLSA->LSHeader.advertisingRouter);
                printf("        linkStateID %u\n",
                    listLSA->LSHeader.linkStateID);
                printf("        attached router %u\n",
                    attachedRtr[i]);
                printf("        networkMask %u\n",
                    netMask);
            }

            tempItem = candidateList->first;

            if (DEBUG_ERROR)
            {
                printf("        NodeAddress = %u\n",
                    candidateListItem->vId);
                printf("        metric = %u\n",
                    candidateListItem->cost);
            }

            // Update candidate List
            while (tempItem)
            {
                MospfVertexItem* tempEntry;
                found = FALSE;

                tempEntry = (MospfVertexItem*) tempItem->data;

                if (DEBUG_ERROR)
                {
                    printf("        tempEntry->vId = %u\n",
                        tempEntry->vId);
                    printf("        tempEntry->parentId = %u\n",
                        tempEntry->parent->vId);
                    printf("        tempEntry->metric = %d\n",
                        tempEntry->cost);
                }

                // If candidate already in candidate list.
                if (tempEntry->vId == candidateListItem->vId)
                {
                    BOOL updateReqd = FALSE;
                    found = TRUE;

                    // Update candidate in candidate list if new
                    // candidate has smaller metric.

                    if (tempEntry->cost > candidateListItem->cost)
                    {
                        updateReqd = TRUE;
                    }

                    // check for multiple posibilities
                    // If costs are equal, then the following
                    // tiebreakers are invoked.

                    else if (tempEntry->cost
                        == candidateListItem->cost)
                    {
                        // The type of Link is compared to W's current
                        // IncomingLinkType, and whichever link has
                        // the preferred type is chosen

                        if (tempEntry->incomingLinkType
                            > candidateListItem->incomingLinkType)
                        {
                            updateReqd = TRUE;
                        }

                        // If the link types are the same,
                        else if (tempEntry->incomingLinkType
                            == candidateListItem->incomingLinkType)
                        {
                            // then a link whose Parent is a transit
                            // network is preferred over one whose Parent
                            // is a router.

                            if (tempEntry->parent->vType
                                < candidateListItem->parent->vType)
                            {
                                updateReqd = TRUE;
                            }

                            // If the links are still equivalent,
                            else if (tempEntry->parent->vType
                                == candidateListItem->parent->vType)
                            {
                                //the link whose Parent has the higher
                                // Vertex ID is chosen.

                                if (tempEntry->parent->vId
                                    < candidateListItem->parent->vId)
                                {
                                    updateReqd = TRUE;

                                }   //check for parent Id

                            }   //check for parent Type

                        }   //check for incoming link

                    }   //check for cost

                    // check existing entry needs to be updated
                    if (updateReqd == TRUE)
                    {
                        tempEntry->vType = candidateListItem->vType ;
                        tempEntry->cost = candidateListItem->cost;

                        tempEntry->parent = candidateListItem->parent;

                        tempEntry->associatedIntf =
                            candidateListItem->associatedIntf;

                        tempEntry->LSA = candidateListItem->LSA;
                        tempEntry->ttl = candidateListItem->ttl;

                        tempEntry->incomingLinkType =
                            candidateListItem->incomingLinkType;


                        if (DEBUG_ERROR)
                        {
                            printf("    updating vertex %u with "
                                "metric %d and parentId %u to "
                                "candidate list\n",
                                tempEntry->vId,
                                tempEntry->cost,
                                tempEntry->parent->vId);
                        }
                    }
                    break;
                }
                tempItem = tempItem->next;
            }

            if (!found)
            {
                if (DEBUG_ERROR)
                {
                    printf("    adding vertex %x with parentId "
                        "%u and metric %u to candidate list\n",
                        candidateListItem->vId,
                        candidateListItem->parent->vId,
                        candidateListItem->cost);
                }

                // New candidate, so add to candidate list
                ListInsert(
                    node, candidateList, 0,
                    (void*) candidateListItem);
            }
            else
            {
                // No need to add to candidate list since
                // already there.

                MEM_free(candidateListItem);
            }
        }
    }

    if (DEBUG_ERROR)
    {
        printf("AFTER updating the candidate list is \n");
        printf("  Size of modified candidate list is %d\n",
            candidateList->size);
        MospfPrintThisList(node, candidateList);
    }
}


//--------------------------------------------------------------------//
// NAME         : MospfCheckForWildCardMulticastReceiver()
// PURPOSE      : Mospf Check For WildCard Multicast Receiver
// ASSUMPTION   : TOS Field remains unaltered
// RETURN VALUE : TRUE  if WildCard Multicast Receiver
//--------------------------------------------------------------------//

static
BOOL MospfCheckForWildCardMulticastReceiver(
    Node* node,
    unsigned int areaId,
    NodeAddress vId)
{
    Ospfv2LinkStateHeader* LSHdr = NULL;
    Ospfv2RouterLSA* LSInfo;

    // Search the LSA listed as Link State Origin for this entry
    LSHdr = MospfSearchLSDBToFindMatchingRouterLSA(node, vId, areaId);

    if (LSHdr != NULL)
    {
        LSInfo = (Ospfv2RouterLSA*) LSHdr;

        if (DEBUG_ERROR)
        {
            printf("   LSInfo found \n");
            printf("   age = %d\n", LSInfo->LSHeader.linkStateAge);
            printf("   M_cast = %d \n",
                Ospfv2OptionsGetMulticast(LSInfo->LSHeader.options));
            printf("   Type = %d\n", LSInfo->LSHeader.linkStateType);
            printf("   Ad_router = %u\n",
                LSInfo->LSHeader.advertisingRouter);
            printf("   LinkStateId = %d\n", LSInfo->LSHeader.linkStateID);
            printf("    wildCardMulticastReceiver = %d\n",
                Ospfv2RouterLSAGetWCMCReceiver(LSInfo->ospfRouterLsa));
        }

        // check the wild card multicast receiver BIT
        if (Ospfv2RouterLSAGetWCMCReceiver(LSInfo->ospfRouterLsa) == TRUE)
        {
            if (DEBUG_ERROR)
            {
                printf(" Node %u inform %u is WCMulticasRecvr\n",
                    node->nodeId, vId);
            }
            return TRUE;
        }
    }

    if (DEBUG_ERROR)
    {
        printf(" Node %u inform %u is not a WCMulticasRecvr\n",
            node->nodeId, vId);
    }

    return FALSE;
}


//--------------------------------------------------------------------//
// NAME         : MospfProcessLabeledVertices()
// PURPOSE      : Check if the vertex is labelled with group or
//                it is wild card multicast receiver
//                If necessary, update forwarding cache downstream.
// RETURN VALUE : Null
//--------------------------------------------------------------------//

static
void MospfProcessLabeledVertices(
                                 Node* node,
                                 MospfVertexItem* tempV,
                                 unsigned int areaId,
                                 MospfForwardingTableRow* fwdTableRowPtr)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);
    if (int(tempV->associatedIntf) == -1)
    {
        return;
    }

    if (DEBUG_ERROR)
    {
        char srcNet[100];

        IO_ConvertIpAddressToString(fwdTableRowPtr->pktSrcAddr, srcNet);
        printf(" Node %u checking for sourcenetwork %u i.e. %s ",
            node->nodeId, fwdTableRowPtr->pktSrcAddr, srcNet);
        printf("and group %u \n", fwdTableRowPtr->groupAddr);
        printf("     for vertex %u\n", tempV->vId);
    }

    // check if this vertex is labelled with group

    // RFC :12.2.6
    // This may be true when V is an inter-area multicast forwarder.
    // listed in the body of a group membership-LSA.

    if (MospfVertexItemLabelledWithGroup(
        node, areaId, tempV, fwdTableRowPtr->groupAddr)
        || MospfCheckForWildCardMulticastReceiver(
        node, areaId, tempV->vId))
    {
        int intfForVertex = tempV->associatedIntf;

        if (DEBUG_ERROR)
        {
            printf("For veretx %x: \n", tempV->vId);
            printf(" intf = %x \n", intfForVertex);
            printf(" calculation is for area %d\n", areaId);
            printf(" interfaceis in area %d\n",
                ospf->iface[intfForVertex].areaId);
        }

        // Check if this vertex is in this area
        // if in same area include entry in down stream

        if (areaId == ospf->iface[intfForVertex].areaId)
        {
            // include this interface in down stream List
            MospfModifyDownstreamlistOfForwardingTable(
                node, tempV, fwdTableRowPtr);
        }   //if in same area;
    }   //if labelled;
}


//--------------------------------------------------------------------//
// NAME        : MospfInitCandidateListUsingSummaryLSA
// PURPOSE     : Examine this area's Summary LSA for inter area routes.
// RETURN      : None.
//--------------------------------------------------------------------//
static
void MospfInitCandidateListUsingSummaryLSA(
    Node* node,
    LinkedList* cList,
    LinkedList* sList,
    NodeAddress sourceNet,
    unsigned int areaId,
    Ospfv2LinkStateType summaryLSAType)
{
    Ospfv2List* list = NULL;
    Ospfv2ListItem* listItem;
    Ospfv2LinkStateHeader* LSHeader;
    char* LSABody;
    MospfVertexItem*  newVertex;

    Ospfv2Data* ospf = (Ospfv2Data*)NetworkIpGetRoutingProtocol(node,
        ROUTING_PROTOCOL_OSPFv2);
    Ospfv2Area* thisArea = Ospfv2GetArea(node, areaId);


    if (summaryLSAType == OSPFv2_NETWORK_SUMMARY)
    {
        list = thisArea->networkSummaryLSAList;
    }
    else if (summaryLSAType == OSPFv2_ROUTER_SUMMARY)
    {
        list = thisArea->routerSummaryLSAList;
    }
    else
    {
        assert(FALSE);
    }

    // check summary LSA list
    listItem = list->first;

    while (listItem)
    {
        Int32 metric;
        int index = 0;

        LSHeader = (Ospfv2LinkStateHeader*) listItem->data;

        // search for the sourceNet
        if (LSHeader->linkStateID == sourceNet)
        {
            LSABody = (char*) (LSHeader + 1);
            index += 4;

            memcpy(&metric, &LSABody[index], sizeof(Int32));
            index += sizeof(Int32);
            metric = metric & 0xFFFFFF;

            // For this summary-link-LSA: check if both
            // a) the MC-bit is set in the LSA's Options field and
            // b) the advertised cost is not equal to LSInfinity

            if (Ospfv2OptionsGetMulticast(LSHeader->options) != 1)
            {
                listItem = listItem->next;
                continue;
            }

            // If cost equal to LSInfinity or age equal to MxAge examine next
            if ((metric == MOSPF_LS_INFINITY)
                || (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) == 
                       (OSPFv2_LSA_MAX_AGE / SECOND)))
            {
                listItem = listItem->next;
                continue;
            }

            if (DEBUG_ERROR)
            {
                printf(" borderRt = %x  informs ",
                    LSHeader->advertisingRouter);
                printf(" to reach net %x reqd metric = %d \n",
                    LSHeader->linkStateID, metric);
            }

            // Process this new vertex
            newVertex = (MospfVertexItem*)
                MEM_malloc(sizeof(MospfVertexItem));

            memset(newVertex, 0, sizeof(MospfVertexItem));

            // then the vertex representing the LSA's advertising
            // area border router is added to the candidate list.
            // An added vertex' state is initialized as:
            // IncomingLinkType set to ILSummary,
            // Cost to whatever is advertised in the LSA,
            // Parent to NULL and
            // AssociatedInterface/Neighbor to NULL.

            newVertex->vType = MOSPF_VERTEX_ROUTER;
            newVertex->vId = LSHeader->advertisingRouter;

            newVertex->associatedIntf =
                (unsigned int) MOSPF_INVALID_INTERFACE;
            newVertex->cost = (unsigned char) metric;
            newVertex->ttl = 0;
            newVertex->incomingLinkType = MOSPF_IL_Summary;

            // set LSA for this vertex
            MospfSetLSAFieldForThisVertex(node, areaId, newVertex);

            if (newVertex->LSA == NULL)
            {
                ERROR_Assert(FALSE, "LSA not found \n");
            }


            if (!MospfInShortestPathList(
                sList, newVertex->vId, OSPFv2_POINT_TO_POINT))
            {
                if (DEBUG_ERROR)
                {
                    printf("Node %u :insert vertex %u in C_List\n",
                        node->nodeId, newVertex->vId);
                    printf(" metric = %d\n", newVertex->cost);
                }

                // insert this matching vertex in Candidate List
                ListInsert(node, cList, 0, (void*) newVertex);
            }
            else
            {
                MEM_free(newVertex);
            }

        }   //if matching entry found

        listItem = listItem->next;
    }
}


//--------------------------------------------------------------------//
// NAME         : MospfInitCandidateListForThisArea()
// PURPOSE      : Initializes Candidate list
// ASSUMPTION   : As there is currently one area, we always consider source
//              : is ItraArea source
// RETURN VALUE : NULL
//--------------------------------------------------------------------//

static
void MospfInitCandidateListForThisArea(
    Node* node,
    LinkedList* cList,
    NodeAddress sourceNet,
    unsigned int areaId)
{
    Ospfv2LinkStateHeader* LSHeader;
    MospfVertexItem* cListItem;
    Ospfv2RoutingTableRow*  newRoute = NULL;
    Ospfv2Data* ospf = (Ospfv2Data*)NetworkIpGetRoutingProtocol(node,
        ROUTING_PROTOCOL_OSPFv2);

   // get the current route & LS Origin from Ospfv2 Routing Table
    newRoute = Ospfv2GetRouteToSrcNet(
                    node, sourceNet,
                        OSPFv2_DESTINATION_NETWORK);

    if (newRoute == NULL)
    {
        if (DEBUG_ERROR)
        {
            printf("Node %u: does not getValidRoute from R-Table\n",
                node->nodeId);
        }
        return;
    }

    LSHeader = (Ospfv2LinkStateHeader*)(newRoute->LSOrigin) ;

    if (DEBUG_ERROR)
    {
        printf("   LSHeader found \n");
        printf("   age = %d\n", LSHeader->linkStateAge);
        printf("   M_cast = %d \n",
            Ospfv2OptionsGetMulticast(LSHeader->options));
        printf("   Type = %d\n", LSHeader->linkStateType);
        printf("   Ad_router = %x\n", LSHeader->advertisingRouter);
        printf("   LinkStateId = %x\n", LSHeader->linkStateID);
    }

    if (Ospfv2MaskDoNotAge(ospf, LSHeader->linkStateAge) >= 
           (OSPFv2_LSA_MAX_AGE / SECOND))
    {
        fflush(stdout);
        ERROR_Assert(FALSE, "Error in LS-Origin Detection\n");
    }

    // Vertex identified by LSA is installed on the List
    cListItem = (MospfVertexItem*) MEM_malloc(sizeof(MospfVertexItem));
    memset(cListItem, 0, sizeof(MospfVertexItem));

    // Set Vertex type
    if (LSHeader->linkStateType == OSPFv2_ROUTER)
    {
        if (DEBUG_ERROR)
        {
            printf("    Vertex %u is of Router Type\n",
                LSHeader->linkStateID);
        }

        cListItem->vType = MOSPF_VERTEX_ROUTER;
    }
    else if (LSHeader->linkStateType == OSPFv2_NETWORK)
    {
        if (DEBUG_ERROR)
        {
            printf("    Vertex %u is of Network Type\n",
                LSHeader->linkStateID);
        }

        cListItem->vType = MOSPF_VERTEX_TRANSIT_NETWORK;
    }
    else
    {
        MEM_free(cListItem);
        return;
    }

    // Set Vertex ID
    cListItem->vId = LSHeader->linkStateID;

    // set LSA for this vertex
    MospfSetLSAFieldForThisVertex(node, areaId, cListItem);

    if (cListItem->LSA == NULL)
    {
        ERROR_Assert(FALSE, "LSA not found \n");
    }

    // This is root of the tree
    cListItem->associatedIntf = (unsigned int) MOSPF_INVALID_INTERFACE;
    cListItem->cost = 0;
    cListItem->ttl = 0;
    cListItem->incomingLinkType = MOSPF_IL_Direct;

    ListInsert(node, cList, 0, (void*) cListItem);

    if (DEBUG_ERROR)
    {
        printf("Node %u has in its candidate List \n", node->nodeId);
        printf("    Vertex ID = %u\n", cListItem->vId);
        printf("    Cost = %d\n", cListItem->cost);

        printf ("   the size of candidate list is = %d\n", cList->size);
    }
}




//--------------------------------------------------------------------//
// NAME        : MospfCalculateShortestPath()
// PURPOSE     :  Calculate shortest path tree for a particular sourceNet
//               Group pair and update forwarding table
// RETURN VALUE: NULL
//--------------------------------------------------------------------//

static
void MospfCalculateShortestPath(
    Node* node,
    unsigned int areaId,
    MospfForwardingTableRow* newEntry)
{
    LinkedList* candidateList;
    LinkedList* shortestPathList;
    MospfVertexItem* tempV;
    Ospfv2AreaAddressRange* addrInfo;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    if (DEBUG)
    {
        printf("Node %u start calculating SP for area 0x%x\n",
            node->nodeId, areaId);
    }

    // Initialize shortestPathList & CandidateList.
    ListInit(node, &shortestPathList);
    ListInit(node, &candidateList);

    // check if the sourcenet is in same area
    addrInfo = Ospfv2GetAddrRangeInfo(node, areaId, newEntry->pktSrcAddr);

    // TBD: we assume external AS feature is not implemented
    // if (source is in external AS)
    //  {
        //RFC :12.2.4
        // MospfInitCandidateList externally
    //}

    // If sourceNet is in same area or the total domain is  not
    // partioned into different area calculate shortest path.
    if ((addrInfo != NULL)
        || (ospf->partitionedIntoArea == FALSE))
    {
        if (DEBUG_ERROR)
        {
            printf("Node %u & source %x both are in area 0x%x\n",
                node->nodeId, newEntry->pktSrcAddr, areaId);
            printf("So initialize CList for Area 0x%x\n", areaId);
        }

        // Initialize candidate list. One or more vertices
        // are initially placed on the candidate list,
        // depending on the location of the sourceNet
        MospfInitCandidateListForThisArea(
            node, candidateList, newEntry->pktSrcAddr, areaId);
    }
    else
    {
        if (DEBUG_ERROR)
        {
            printf("Node %u & source %u are in different area \n",
                node->nodeId, newEntry->pktSrcAddr);
            printf(" So initialize CList for Area 0x%x\n", areaId);
        }

        // SourceNet belongs to an OSPF area that is not directly attached
        // to the calculating router (RTX). The candidate list is then
        // initialized as follows.

        // Initialize candidtae list using summary LSA
        MospfInitCandidateListUsingSummaryLSA(
            node, candidateList, shortestPathList, newEntry->pktSrcAddr,
            areaId, OSPFv2_NETWORK_SUMMARY);
    }

    while (candidateList->size != 0)
    {
        // Select vertex which have smallest cost value from the
        // candidate list. Move it to shortest path tree list

        tempV = MospfUpdateShortestPathListForThisArea(
                    node, candidateList, shortestPathList);
        ERROR_Assert (tempV != NULL, "No vertex present\n");

        if (DEBUG_ERROR)
        {
            printf(" Node %u \n",node->nodeId);
            printf("    size of SP list after updating is %d \n",
                shortestPathList->size);
            MospfPrintThisList(node, shortestPathList);
        }

        // From: RFC 12.2 (4)
        // Determine whether Vertex V has been labelled
        // with the Destination multicast Group G.
        // If so, it may cause the forwarding cache entry's
        // list of outgoing interfaces/neighbors to be updated
        MospfProcessLabeledVertices(node, tempV, areaId, newEntry);

        // Check vertex V's LSAs to find its neighbors for possible
        // inclusion in the Candidate list
        if (tempV->vType == MOSPF_VERTEX_ROUTER)
        {
            // Vertex is a router
            MospfUpdateCandidateListUsingRouterLSA(
                node, areaId, shortestPathList, candidateList, tempV);
        }
        else
        {
            // Vertex is a network
            MospfUpdateCandidateListUsingNetworkLSA(
                node, areaId, shortestPathList, candidateList, tempV);
        }
        if (DEBUG_ERROR)
        {
            printf("    size of candidate list after updating is %d \n",
                candidateList->size);
        }
    }   //end of while

    // after shortest path is completely updated, prune those vertices
    // which are not labelled with group, and modify downstream if necessary
    //MospfPruneNonLabeledVertices(node, shortestPathList, areaId, newEntry);

    if (DEBUG)
    {
        printf("Node %u updated SP for area 0x%x\n",
            node->nodeId, areaId);
        MospfPrintThisList(node, shortestPathList);
    }

    //Update Fwd Cache entry's upstream Node
    MospfModifyUpstreamNode(node, shortestPathList, areaId, newEntry);

    ListFree(node, shortestPathList, FALSE);
    ListFree(node, candidateList, FALSE);
}


//--------------------------------------------------------------------//
// NAME     : MospfLookUpForwardingTableEntry()
// PURPOSE  :  Search the forwarding cache for given source-group pair
// RETURN   : MospfForwardingTableRow*
//--------------------------------------------------------------------//

static
MospfForwardingTableRow* MospfLookUpForwardingTableEntry(
    Node* node,
    NodeAddress sourceAddr,
    NodeAddress destAddr)
{
    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node,
        MULTICAST_PROTOCOL_MOSPF);

    MospfForwardingTable* forwardingTable = &mospf->forwardingTable;
    MospfForwardingTableRow*  rowPtr;

    unsigned int i;
    rowPtr = NULL;



    // Search forwarding Table for the desired match
    for (i = 0; i < forwardingTable->numRows; i++)
    {
        if ((forwardingTable->row[i].groupAddr == destAddr)
            && (forwardingTable->row[i].pktSrcAddr == sourceAddr))
        {
            if (DEBUG)
            {
                printf("    Source group pair found in Forwarding Table\n");
            }

            rowPtr = &forwardingTable->row[i];
            return rowPtr;
        }
    }//end of for;

    return NULL;
}


//--------------------------------------------------------------------//
// NAME       : IsInterfaceInDownstreamList
// PURPOSE    : Check if the interfaceId is already included in the
//             downstreamList of Mospf forwarding table for a
//             particular source group pair
// RETURN     :if interface found return RotingMospfDownStream
//--------------------------------------------------------------------//

static
MospfdownStream* IsInterfaceInDownstreamList(
    MospfForwardingTableRow* entry,
    int interfaceIndex)
{
    ListItem* tempListItem;

    tempListItem = entry->downStream->first;

    while (tempListItem)
    {
        MospfdownStream* downStream = (MospfdownStream*) tempListItem->data;

        if (downStream->interfaceIndex == interfaceIndex)
        {
            if (DEBUG_ERROR)
            {
                printf(" the interfaceId %d is already present \n",
                    downStream->interfaceIndex);
            }

            return downStream;
        }

        tempListItem = tempListItem->next;
    }//End while
    return NULL;
}


//--------------------------------------------------------------------//
// NAME         : MospfModifyForwardingTableUsingLocalDatabase()
// PURPOSE      : Modify the forwarding Table too deliver multicast
//                packet to local group using local group dataBase
//                available from IGMP
// RETURN VALUE : NULL
//--------------------------------------------------------------------//

static
void MospfModifyForwardingTableUsingLocalDatabase(
    Node* node,
    MospfForwardingTableRow*  newEntry)
{
    int j;
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);


    // update cache entry using local group database
    for (j = 0; j < node->numberInterfaces; j++)
    {
        BOOL localMember = FALSE;

        IgmpSearchLocalGroupDatabase(
            node, newEntry ->groupAddr, j, &localMember);

        if (DEBUG_ERROR)
        {
            char clockTime[100];
            ctoa(node->getNodeTime(), clockTime);
            printf("Node %u: Report from Igmp for Src %u & grp %u\n",
                node->nodeId, newEntry->pktSrcAddr,
                newEntry->groupAddr);
            printf(" at %s, localMember (: %d) at interface %d\n",
                clockTime, localMember, j);
        }

        if ((localMember == TRUE)
                && ip->interfaceInfo[j]->multicastProtocolType
                    == MULTICAST_PROTOCOL_MOSPF
            && (IsInterfaceInDownstreamList(newEntry, j) == NULL))
        {
            MospfdownStream* downStreamListPtr =
                (MospfdownStream*) MEM_malloc(sizeof(MospfdownStream));

            downStreamListPtr->interfaceIndex = j;
            downStreamListPtr->ttl = 1;

            if (DEBUG)
            {
                printf("Node %u update fwd Cache entry for local-Group "
                    "and include %d intf in dn_stream List\n",
                    node->nodeId, downStreamListPtr->interfaceIndex);
            }

            ListInsert(node, newEntry->downStream, 0,
                (void*) downStreamListPtr);
        }
    }   //end of for;
}


//--------------------------------------------------------------------//
// NAME        : MospfBuiltNewEntry()
// PURPOSE     : Create a new entry in forwarding cache
// RETURN VALUE: ForwardingCacheRow*
//--------------------------------------------------------------------//

static
MospfForwardingTableRow* MospfBuiltNewEntry(
    Node* node,
    NodeAddress sourceNet,
    NodeAddress grpAddr,
#ifdef ADDON_DB
    NodeAddress netMask,
    NodeAddress srcNodeAddr,
    int pktIncomingIntf,
    NodeAddress prevHopAddress)
#else
    NodeAddress netMask)
#endif

{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);

    MospfForwardingTable* frwdTable = &mospf->forwardingTable;
    MospfForwardingTableRow* newEntry;
    Ospfv2ListItem* areaListItem;

    unsigned int i = frwdTable->numRows;

    // Check if the table has been filled
    if (frwdTable->numRows == frwdTable->allottedNumRow)
    {
        MospfForwardingTableRow* oldRow = frwdTable->row;
        int size = 2 * frwdTable->allottedNumRow
                    * sizeof(MospfForwardingTableRow);

        // Double forwarding table size
        frwdTable->row = (MospfForwardingTableRow*) MEM_malloc(size);
        memset(frwdTable->row, 0, size);

        for (i = 0; i < frwdTable->numRows; i++)
        {
            frwdTable->row[i] = oldRow[i];
        }

        MEM_free(oldRow);

        // Update table size
        frwdTable->allottedNumRow *= 2;
    }   // if table is filled

    newEntry = &frwdTable->row[i];

    // Insert new row
    newEntry->groupAddr = grpAddr;
    newEntry->pktSrcAddr = sourceNet;
    newEntry->sourceAddressMask = netMask;
    newEntry->incomingIntf = MOSPF_INVALID_INTERFACE;
    newEntry->upStream = (unsigned int) MOSPF_UNREACHABLE_NODE;
    newEntry->rootArea = OSPFv2_INVALID_AREA_ID;
    newEntry->rootCost = MOSPF_LS_INFINITY;

#ifdef ADDON_DB
    newEntry->srcNodeAddr = srcNodeAddr;
    newEntry->pktIncomingIntf = pktIncomingIntf;
    newEntry->prevHopAddress = prevHopAddress;
#endif

    // Initializes downStream
    ListInit(node, &newEntry->downStream);

    frwdTable->numRows++;

    if (DEBUG_ERROR)
    {
        printf("    NumRows in forwarding table = %d\n", frwdTable->numRows);
        printf("    Forwarding Table is updated for source %u & group %u\n",
            sourceNet, grpAddr);
    }

    //CALCULATION OF SHORTEST PATH
    areaListItem = ospf->area->first;

    while (areaListItem)
    {
        Ospfv2Area* thisArea =
            (Ospfv2Area*) areaListItem->data;

        if (DEBUG)
        {
            printf("Node %u: need to update FWD cache entry "
                "and calculate SP for Area 0x%x\n",
                node->nodeId, thisArea->areaID);
        }

        MospfCalculateShortestPath(
            node, thisArea->areaID, newEntry);

        areaListItem = areaListItem->next;
    }   //for all area;

    // update cache entry using local group database
    MospfModifyForwardingTableUsingLocalDatabase(node, newEntry);

    if (DEBUG)
    {
        printf("Node %u FwdTable is updated for source %u & group %u\n",
            node->nodeId, sourceNet, grpAddr);
    }

    // updated forwarding table is returned to forwarding function//
    return newEntry;
}


//--------------------------------------------------------------------//
// NAME        : MospfModifyDownstreamlistOfForwardingTable()
// PURPOSE     : Check if the node has any downstrem, hence to modify
//               the downstream list of it's forwarding cache entry.
// RETURN VALUE: None
//--------------------------------------------------------------------//

static
void MospfModifyDownstreamlistOfForwardingTable(
    Node* node,
    MospfVertexItem* tempV,
    MospfForwardingTableRow* fwdTableRowPtr)
{
    MospfdownStream* downStreamListPtr;
    MospfdownStream* oldPtr = NULL;
    int interfaceIndex;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    interfaceIndex = tempV->associatedIntf;

    // check if this interface is not the incoming interface
    // or the vertex is not one of it's own interfaceAddress

    if ((ospf->iface[interfaceIndex].address == tempV->vId)
        || (tempV->associatedIntf == (unsigned) MOSPF_UNREACHABLE_NODE))
    {
        if (DEBUG_ERROR)
        {
            printf(" No modification in Dn_stream list for \n");
            printf(" vertex %u & interface %d \n",
                tempV->vId, interfaceIndex);
        }
        return;
    }

    if (DEBUG_ERROR)
    {
        printf(" the just added entry is:\n");
        printf(" interfaceIndex = %u \n", interfaceIndex);
        printf(" TTL = %u \n", tempV->ttl);
        printf(" interfaceAddress = %u \n", tempV->associatedIntf);
    }

    oldPtr = IsInterfaceInDownstreamList(fwdTableRowPtr, interfaceIndex);

    if (oldPtr == NULL)
    {
        downStreamListPtr = (MospfdownStream*)
            MEM_malloc(sizeof(MospfdownStream));

        downStreamListPtr->interfaceIndex = interfaceIndex;
        downStreamListPtr->ttl = tempV->ttl;

        if (DEBUG_ERROR)
        {
            printf("Node %u include intf %d in dn_strm List",
                node->nodeId, downStreamListPtr->interfaceIndex);
            printf(" for labelled vertex %x\n", tempV->vId);
        }

        ListInsert(
            node, fwdTableRowPtr->downStream,
            0, (void*) downStreamListPtr);
    }
    else
    {
        if (oldPtr->ttl > tempV->ttl)
        {
            oldPtr->ttl = tempV->ttl;
        }
    }
}


//--------------------------------------------------------------------//
// NAME         : MospfModifyShortestPathAndForwardingTable()
// PURPOSE      : Modify the entry associated with this group in the
//                shortest path tree and the forwarding table entry
//                for this group
// RETURN VALUE : NULL
//--------------------------------------------------------------------//

void MospfModifyShortestPathAndForwardingTable(Node* node)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);
    MospfForwardingTable* forwardingTable = &mospf->forwardingTable;
    MospfForwardingTableRow*  rowPtr = NULL;

    unsigned int i;
    NodeAddress sourceNet;
    Ospfv2RoutingTableRow* newRoute;
    Ospfv2ListItem* areaListItem;

    if (DEBUG)
    {
        if (forwardingTable->numRows != 0)
        {
            char clockTime[100];
            ctoa(node->getNodeTime(), clockTime);
            printf("Node %u LSDB is updated: \n", node->nodeId);
        }
    }

    for (i = 0; i < forwardingTable->numRows; i++)
    {
        rowPtr = &forwardingTable->row[i];

        if (DEBUG_ERROR)
        {
            printf("updates F_Table for Src %d and group %u\n",
                rowPtr->pktSrcAddr, rowPtr->groupAddr);
        }

        // Clear the previous Dowmstresm List And fill it with
        // data associated with latest Link state status & using
        // the corresponding updated shortest path tree for this
        // source -group pair

#ifdef ADDON_DB
        StatsDBConnTable::MulticastConnectivity stats;
        stats.nodeId = node->nodeId;
        stats.destAddr = rowPtr->groupAddr;
        strcpy(stats.rootNodeType,"Source");
        stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                    rowPtr->srcNodeAddr);

        if (node->nodeId != stats.rootNodeId)
        {
            stats.upstreamNeighborId =
                     MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                    rowPtr->upStream);
            stats.upstreamInterface = rowPtr->incomingIntf;
        }
        else
        {
            stats.upstreamNeighborId = stats.rootNodeId;
            //my own packet...add interface index as CPU_INTERFACE
            stats.upstreamInterface = CPU_INTERFACE;
        }

        ListItem* tempListItem;
        tempListItem = rowPtr->downStream->first;

        while (tempListItem)
        {
            MospfdownStream* downStreamInfo =
                (MospfdownStream*) tempListItem->data;

            stats.outgoingInterface = downStreamInfo->interfaceIndex;

            //delete this entry from multicast_connectivity cache
            STATSDB_HandleMulticastConnDeletion(node,stats);

            tempListItem = tempListItem->next;
        }
#endif

        ListFree(node, rowPtr->downStream, FALSE);
        ListInit(node, &rowPtr->downStream);

        // get the current route from Ospfv2 routing Table
        newRoute = Ospfv2GetRouteToSrcNet(
                        node, rowPtr->pktSrcAddr,
                        OSPFv2_DESTINATION_NETWORK);

        if (newRoute == NULL)
        {
            if (DEBUG_ERROR)
            {
                printf("Node %u: does not getValidRoute from R-Table\n",
                    node->nodeId);
            }
            continue;
        }

        if (DEBUG_ERROR)
        {
            printf(" Node %u informs %dth interface state %d\n",
                node->nodeId, rowPtr->incomingIntf,
                ospf->iface[rowPtr->incomingIntf].state);
        }

        // If interface state is down, no link should be added
        if (rowPtr->incomingIntf >= 0 &&
            ospf->iface[rowPtr->incomingIntf].state < S_PointToPoint)
        {
            // Clear the previous Upstream
            rowPtr->upStream = (unsigned int) MOSPF_UNREACHABLE_NODE;
            rowPtr->incomingIntf = MOSPF_INVALID_INTERFACE;
            rowPtr->rootArea = OSPFv2_INVALID_AREA_ID;
            rowPtr->rootCost = MOSPF_LS_INFINITY;
            rowPtr->sourceAddressMask = newRoute->addrMask;


            if (DEBUG)
            {
                printf("Node %u inform new Intf setting is required\n",
                    node->nodeId);
            }
        }

        sourceNet = rowPtr->pktSrcAddr;

        if (DEBUG)
        {
            char srcNet[100];
            IO_ConvertIpAddressToString(sourceNet, srcNet);
            printf(" sourcenetwork %u i.e. %s \n", sourceNet, srcNet);
        }

        // CALCULATION OF SHORTEST PATH
        areaListItem = ospf->area->first;

        while (areaListItem)
        {
            Ospfv2Area* thisArea =
                (Ospfv2Area*) areaListItem->data;

            if (DEBUG)
            {
                printf("Node %u: need to update FWD cache entry "
                    "and calculate SP for Area 0x%x\n",
                    node->nodeId, thisArea->areaID);
            }

            MospfCalculateShortestPath(
                node, thisArea->areaID, rowPtr);

            areaListItem = areaListItem->next;
        }   //for all area;

        // update cache entry using local group database
        MospfModifyForwardingTableUsingLocalDatabase(node, rowPtr);

        if (DEBUG)
        {
            printf("Node %u F_Table is updated for src %u & grp %u\n",
                node->nodeId, rowPtr->pktSrcAddr, rowPtr->groupAddr);
        }
    }   //end of for;

    if (DEBUG_ERROR)
    {
        MospfPrintForwardingTable(node);
    }
}


//--------------------------------------------------------------------//
// NAME     : MospfInit
// PURPOSE  : Initialize the mospf routing protocol.
//            It initialize different parameters specially
//            required to handle Multicasting features.
// RETURN   : NONE.
//--------------------------------------------------------------------//

void MospfInit(
    Node* node,
    const NodeInput* nodeInput,
    int interfaceIndex)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;

    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    MospfData* mospf = (MospfData*) MEM_malloc(sizeof(MospfData));

    // This part is done to inform Ospf about the multicast protocol Mospf
    ospf->isMospfEnable = TRUE;
    ospf->multicastRoutingProtocol = (void*) mospf;

    memset(mospf, 0, sizeof(MospfData));

    RANDOM_SetSeed(mospf->seed,
                   node->globalSeed,
                   node->nodeId,
                   MULTICAST_PROTOCOL_MOSPF,
                   interfaceIndex);

    // Determine whether or not to print the stats at the end of simulation.
    IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput,
        "ROUTING-STATISTICS", &retVal, buf);

    if ((retVal == FALSE) || (strcmp (buf, "NO") == 0))
    {
        mospf->showStat = FALSE;
    }
    else if (strcmp (buf, "YES") == 0)
    {
        mospf->showStat = TRUE;
    }

    // Set MOSPF layer pointer
    NetworkIpSetMulticastRoutingProtocol(node, mospf, interfaceIndex);

    NetworkIpSetMulticastRouterFunction(
        node, &MospfForwardingFunction, interfaceIndex);

    // Initializes statistics structure
    MospfInitStat(node);

    // Initialize Mospf Interface Structure
    MospfInitInterface(node);

    // Inform IGMP about multicast routing protocol
    if (ip->isIgmpEnable == TRUE
        && !TunnelIsVirtualInterface(node, interfaceIndex))
    {
        IgmpSetMulticastProtocolInfo(
            node, interfaceIndex, &MospfLocalMembersJoinOrLeave);
    }

    // Initialize Mospf forwarding table
    MospfInitForwardingTable(node);

    // modify this entry when domain is partioned into area
    if (ospf->partitionedIntoArea == TRUE)
    {
        // check if this node is interAreaMulticast Forwarder
        MospfCheckForInterAreaMulticastFwder(node, nodeInput);

        if (mospf->interAreaMulticastFwder == TRUE)
        {
            if (ospf->areaBorderRouter != TRUE)
            {
                char errStr[MAX_STRING_LENGTH];

                sprintf(errStr, "Node %u is not an area border router\n"
                                "It can't be configured as I_A_M_F\n",
                        node->nodeId);
                ERROR_ReportWarning(errStr);

                mospf->interAreaMulticastFwder = FALSE;
            }
            else
            {
                if (DEBUG)
                {
                    printf("Node %u is configured as I_A_M_F\n",
                        node->nodeId);
                }
                mospf->interAreaMulticastFwder = TRUE;
            }
        }
    }   //ifpartioned into area;

    if (DEBUG)
    {
        FILE* fp;
        fp = fopen("FWDTable.txt", "w");
        ERROR_Assert(fp, "Couldn't open file FWDTable.txt\n");

        if (fp == NULL)
        {
            fprintf(stdout, "Couldn't open file FWDTable.txt\n");
        }
    }

    if (DEBUG)
    {
        int i;
        for (i = 0; i < node->numberInterfaces; i++)
        {
            NodeAddress addr = NetworkIpGetInterfaceAddress(node, i);

            printf("Node %u : IP of interface %d = %x\n",
                node->nodeId, i, addr);
        }
    }
}


//--------------------------------------------------------------------//
// NAME         :MospfLocalMembersJoinOrLeave()
//
// PURPOSE      :To Handle local group membership events using Igmp.
//               This function will be called from IGMP whenever a new
//               group membership was cretaed in local network or
//               an existing membership was removed (i.e. no member
//               for this group was exist).
//
// PARAMETERS   : 1) Pointer to node.
//                2) Group for which the event is generated.
//                3) Interface to connected network.
//                4) Event type (Join/Leave).
//
// RETURN VALUE :NULL
//--------------------------------------------------------------------//

void MospfLocalMembersJoinOrLeave(
    Node* node,
    NodeAddress groupAddr,
    int interfaceId,
    LocalGroupMembershipEventType event)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    MospfData* mospf = (MospfData*) NetworkIpGetMulticastRoutingProtocol(
                                        node,
                                        MULTICAST_PROTOCOL_MOSPF);

    if (DEBUG)
    {
        char clockStr[MAX_STRING_LENGTH];
        char grpAddrStr[MAX_STRING_LENGTH];
        char ipAddrStr[MAX_STRING_LENGTH];
        char eventStr[MAX_STRING_LENGTH];
        char drStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        IO_ConvertIpAddressToString(groupAddr, grpAddrStr);
        IO_ConvertIpAddressToString(ospf->iface[interfaceId].address,
                                    ipAddrStr);
        IO_ConvertIpAddressToString(
            ospf->iface[interfaceId].designatedRouterIPAddress,
            drStr);

        if (event == LOCAL_MEMBER_JOIN_GROUP)
        {
            sprintf(eventStr, "JoinGroup");
        }
        else if (event == LOCAL_MEMBER_LEAVE_GROUP)
        {
            sprintf(eventStr, "LeaveGroup");
        }
        else
        {
            sprintf(eventStr, "Unknown");
        }

        printf("Node %u: Igmp notify about group-membership event\n"
                "    Group Addr = %s\n"
                "    Interface = %s\n"
                "    Event Type = %s\n"
                "    Current Time = %s\n"
                "    DR on this interface = %s\n",
            node->nodeId, grpAddrStr, ipAddrStr,
            eventStr, clockStr, drStr);
    }

    // update the statistics
    if (event == LOCAL_MEMBER_JOIN_GROUP)
    {
        mospf->stats.numGroupJoin++;
    }
    else if (event == LOCAL_MEMBER_LEAVE_GROUP)
    {
        mospf->stats.numGroupLeave++;
    }


    // RFC 1584:
    // A router will originate a group membership-LSA for multicast group
    // when the router is Designated Router on a network If associated
    // interface is a broadcast network
    //
    // thus if router is not the Designated-Rtr for the network discard it

    if ((ospf->iface[interfaceId].type == OSPFv2_BROADCAST_INTERFACE)
        && (ospf->iface[interfaceId].designatedRouterIPAddress
                != ospf->iface[interfaceId].address))
    {
        if (DEBUG)
        {
            printf("    I'm not D_Rtr. So no need to generate LSA\n\n");
        }
        return;
    }

    // check the IGMP event and behave accordingly
    switch (event)
    {
        // A new group membership has been just created
        case LOCAL_MEMBER_JOIN_GROUP:
        {
            if (DEBUG)
            {
                printf("    Time to originate a new groupMembership LSA\n");
            }

            // Schedule to originate group Membership LSA
            MospfScheduleGroupMembershipLSA(
                node, ospf->iface[interfaceId].areaId, interfaceId,
                groupAddr, FALSE);

            break;
        }   //end of case 1;

        // When the last group member of the desired group
        // leave a particular interface of a node
        case LOCAL_MEMBER_LEAVE_GROUP:
        {
            if (DEBUG)
            {
                printf("    Time to age out groupMembership LSA \n");
            }

            // Schedule to originate group Membership LSA
            MospfScheduleGroupMembershipLSA(
                node, ospf->iface[interfaceId].areaId, interfaceId,
                groupAddr, TRUE);

            break;
        }

        default:
        {
            ERROR_Assert(FALSE, "Unknown local membership event\n");
        }
    }    // end of switch;
}


//--------------------------------------------------------------------//
// NAME      : MospfForwardingFunction()
//
// PURPOSE   : Address of this function is assigned in the IP structure
//             during initialization.IP forwards packet by getting the
//             nextHop from this function. The fuction will be called
//             via a pointer (RouterFunction in the IP structure) from
//             the function RoutePacketAndSendToMac() in ip.c.
//
// RETURN    : None.
//
// NOTE      : Packet is from IP with IP header already created.
//             ipHeader->ip_dst specifies the multicast destination.
//--------------------------------------------------------------------//

void MospfForwardingFunction(
    Node* node,
    Message* msg,
    GROUP_ID destAddr,
    int interfaceId,
    BOOL* packetWasRouted,
    NodeAddress prevHop)
{
    Ospfv2Data* ospf = (Ospfv2Data*)
        NetworkIpGetRoutingProtocol(node, ROUTING_PROTOCOL_OSPFv2);

    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);

    MospfForwardingTableRow* forwardingTableRowPtr;
    int i;
    NodeAddress sourceNet;
    NodeAddress nextHop;
    ListItem* tempListItem;
    Ospfv2RoutingTableRow* oldRoute;
    BOOL originateByMe = FALSE;
    BOOL isloopbackPacket = FALSE;

    IpHeaderType* ipHeader = (IpHeaderType*) MESSAGE_ReturnPacket(msg);

    if (DEBUG)
    {
        char clockStr[100];
        char grpAddrStr[100];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        IO_ConvertIpAddressToString(destAddr, grpAddrStr);

        printf("Node %u get m_cast pkt for dest %s at %s from intf %d\n",
            node->nodeId, grpAddrStr, clockStr, interfaceId);
        printf("Source %x\n", ipHeader->ip_src);
    }

    mospf->stats.numOfMulticastPacketReceived++;

    // If multicast address falls in the reserved range
    // no needto forward it discard packet scilently

    if ((destAddr > IP_MIN_RESERVED_MULTICAST_ADDRESS)
        && (destAddr <= IP_MAX_RESERVED_MULTICAST_ADDRESS))
    {
        char errStr[MAX_STRING_LENGTH];

        sprintf(errStr, "Node %u discard multicast packet %u as it is "
            "in reserved range\n", node->nodeId, destAddr);

        ERROR_ReportWarning(errStr);

        mospf->stats.numOfMulticastPacketDiscard++;
#ifdef ADDON_DB
        mospf->mospfMulticastNetSummaryStats.m_NumDataRecvd++;
        mospf->mospfMulticastNetSummaryStats.m_NumDataDiscarded++;
#endif
        return;
    }

    // If TTL value is less than 1, discard packet scilently
    if (ipHeader->ip_ttl < 1)
    {
        if (DEBUG)
        {
            printf("Node %u discard multicast packet as TTL of "
                "packet is < 1\n\n", node->nodeId);
        }
        mospf->stats.numOfMulticastPacketDiscard++;
#ifdef ADDON_DB
        mospf->mospfMulticastNetSummaryStats.m_NumDataRecvd++;
        mospf->mospfMulticastNetSummaryStats.m_NumDataDiscarded++;
#endif
        return;
    }

    // If packet is from transport layer, send out directly to the interface
    // specified by the upper layer and then treat the packet as if it was
    // received over that interface
    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ipHeader->ip_src == ospf->iface[i].address)
        {
            if (DEBUG)
            {
                printf(" Node %u: I am the source of the packet\n",
                    node->nodeId);
                printf(" forward packet through default intf %d \n",i);
            }
            originateByMe = TRUE;
            nextHop = NetworkIpGetInterfaceBroadcastAddress(node, i);

            NetworkIpSendPacketToMacLayer(node,
                MESSAGE_Duplicate(node, msg),
                i,
                nextHop);

            mospf->stats.numOfMulticastPacketGenerated++;

#ifdef ADDON_DB
            //this check is to avoid the counter increase for loopback packet
            if (prevHop == ANY_IP && msg->originatingNodeId == node->nodeId)
            {
                mospf->mospfMulticastNetSummaryStats.m_NumDataSent++;
            }

            StatsDBConnTable::MulticastConnectivity stats;

            stats.nodeId = node->nodeId;
            stats.destAddr = destAddr;
            strcpy(stats.rootNodeType,"Source");
            stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                           ipHeader->ip_src);
            stats.outgoingInterface = i;

            if (node->nodeId != stats.rootNodeId)
            {
                stats.upstreamNeighborId =
                        MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                              prevHop);
                stats.upstreamInterface = interfaceId;
            }
            else
            {
                //my own packet
                stats.upstreamNeighborId = stats.rootNodeId;
                //add interface index as CPU_INTERFACE
                stats.upstreamInterface = CPU_INTERFACE;
            }

            // insert this new entry into multicast_connectivity cache
            STATSDB_HandleMulticastConnCreation(node,stats);
#endif
        }
    }

    if (prevHop != ANY_IP && msg->originatingNodeId == node->nodeId)
    {
        isloopbackPacket = TRUE;
    }
#ifdef ADDON_DB
    if (!originateByMe)
    {
        mospf->mospfMulticastNetSummaryStats.m_NumDataRecvd++;
    }
    else
    {
        if (isloopbackPacket &&
            NetworkIpIsPartOfMulticastGroup(node, ipHeader->ip_dst))
        {
            mospf->mospfMulticastNetSummaryStats.m_NumDataRecvd++;
        }
    }
#endif

    // Identify the source Network
    if (DEBUG_ERROR)
    {
        Ospfv2PrintRoutingTable(node);
    }

    oldRoute = Ospfv2GetRouteToSrcNet(
        node, ipHeader->ip_src, OSPFv2_DESTINATION_NETWORK);

    if (oldRoute == NULL)
    {
        if (DEBUG)
        {
            printf("Node %u does not have Valid Route to source\n"
                "    Discard received packet!\n" ,
                node->nodeId);
        }

        mospf->stats.numOfMulticastPacketDiscard++;
#ifdef ADDON_DB
        if (!originateByMe)
        {
            mospf->mospfMulticastNetSummaryStats.m_NumDataDiscarded++;
        }
#endif
        return;
    }

    if (oldRoute->pathType > OSPFv2_INTER_AREA)
    {
        char errStr[MAX_STRING_LENGTH];

        sprintf(errStr, "Node %u & Src %u are not in same AS\n",
            node->nodeId, ipHeader->ip_src);
        ERROR_ReportWarning(errStr);
    }

    sourceNet = MaskIpAddress(ipHeader->ip_src, oldRoute->addrMask);

    // Looking for mathching entry in forwarding cache
    forwardingTableRowPtr = MospfLookUpForwardingTableEntry(
        node,
        sourceNet,
        destAddr);

    // If no entry found for this source - group pair, build one
    if (forwardingTableRowPtr == NULL)
    {
        if (DEBUG)
        {
            printf("Time to built new entry in forwarding cache \n");
        }

        forwardingTableRowPtr = MospfBuiltNewEntry(
            node,
            sourceNet,
            destAddr,
#ifdef ADDON_DB
            oldRoute->addrMask,
            ipHeader->ip_src,
            interfaceId,
            prevHop);
#else
            oldRoute->addrMask);
#endif
    }
    if (forwardingTableRowPtr->incomingIntf != interfaceId)
    {
        if (DEBUG_ERROR)
        {
            printf("Node %u discard pkt from wrong interface \n",
                node->nodeId);
            printf(" incoming Intf %d \n",interfaceId);
            printf(" actual Intf is %d\n",
                forwardingTableRowPtr->incomingIntf);
        }

        mospf->stats.numOfMulticastPacketDiscard++;
#ifdef ADDON_DB
        if (!originateByMe)
        {
            mospf->mospfMulticastNetSummaryStats.m_NumDataDiscarded++;
        }
#endif
        return;
    }

    // TBD  -> Currently not used
    // else if (upStream == EXTERNEL)
    // {
    //   //TBD
    // }

    if (DEBUG_ERROR)
    {
        MospfPrintForwardingTable(node);
    }

    // Now, forward the packet
    if (DEBUG_ERROR)
    {
        printf("node %u has forwardingtable downstraem of size = %d \n",
            node->nodeId, forwardingTableRowPtr->downStream->size);
    }

    tempListItem = forwardingTableRowPtr->downStream->first;

    while (tempListItem)
    {
        MospfdownStream* downStreamInfo =
            (MospfdownStream*) tempListItem->data;

        // check if this interface is the incoming interface
        if (downStreamInfo->interfaceIndex ==
                forwardingTableRowPtr->incomingIntf)
        {
            //  no need to forward the packet through this interface
            if (DEBUG)
            {
                printf(" Node %u discard interface %d \n",
                    node->nodeId, downStreamInfo->interfaceIndex);
            }

            tempListItem = tempListItem->next;
            continue;
        }

        MospfSetInterfaceLinkTypeAndForwardingType(node,
            downStreamInfo->interfaceIndex);

        // At present this check is not important
        // because we assume all routers are multicast capable

        if (mospf->mInterface[downStreamInfo->interfaceIndex]
            .forwardingType != DATA_LINK_MULTICAST)
        {
            if (DEBUG)
            {
                printf("Node %u discard multicast packet because  "
                    "forwarding type is not DATA_LINK_MULTICAST\n\n",
                    node->nodeId);
            }

            mospf->stats.numOfMulticastPacketDiscard++;
            *packetWasRouted = TRUE;
            tempListItem = tempListItem->next;
            continue;
        }

        nextHop = NetworkIpGetInterfaceBroadcastAddress(
            node, downStreamInfo->interfaceIndex);

#ifdef ADDON_DB
        StatsDBConnTable::MulticastConnectivity stats;

        stats.nodeId = node->nodeId;
        stats.destAddr = destAddr;
        strcpy(stats.rootNodeType,"Source");
        stats.rootNodeId = MAPPING_GetNodeIdFromInterfaceAddress(node,
                                                           ipHeader->ip_src);

        stats.outgoingInterface = downStreamInfo->interfaceIndex;

        if (node->nodeId != stats.rootNodeId)
        {
            stats.upstreamNeighborId =
                         MAPPING_GetNodeIdFromInterfaceAddress(node,prevHop);
            stats.upstreamInterface = interfaceId;
        }
        else
        {
            stats.upstreamNeighborId = stats.rootNodeId;
            //my own packet...add interface index as CPU_INTERFACE
            stats.upstreamInterface = CPU_INTERFACE;
        }

        // if we are using database, update the multicast connection table
        STATSDB_HandleMulticastConnCreation(node,stats);
#endif

        NetworkIpSendPacketToMacLayer(
            node, MESSAGE_Duplicate(node, msg),
            downStreamInfo->interfaceIndex, nextHop);

        if (DEBUG)
        {
            printf("Node %u forwards m_cast pkt thru %dth intf\n\n",
                node->nodeId, downStreamInfo->interfaceIndex);
        }

        tempListItem = tempListItem->next;
        mospf->stats.numOfMulticastPacketForwarded++;
#ifdef ADDON_DB
        if (!originateByMe)
        {
            mospf->mospfMulticastNetSummaryStats.m_NumDataForwarded++;
        }
        else
        {
            if (isloopbackPacket)
            {
                mospf->mospfMulticastNetSummaryStats.m_NumDataForwarded++;
            }
        }
#endif
    }  //End while

    *packetWasRouted = TRUE;
    MESSAGE_Free(node, msg);
}


//--------------------------------------------------------------------//
// NAME        :  MospfFinalize(Node* node)
// PURPOSE     : Provide the final statistical information of the simulation.
// RETURN      : NULL.
//--------------------------------------------------------------------//

void MospfFinalize(Node* node)
{
    MospfData* mospf = (MospfData*)
        NetworkIpGetMulticastRoutingProtocol(node, MULTICAST_PROTOCOL_MOSPF);

    // Only print statistics once per node
    if (mospf->stats.alreadyPrinted == TRUE)
    {
        return;
    }

    mospf->stats.alreadyPrinted = TRUE;

    if (DEBUG)
    {
        MospfPrintForwardingTable(node);
        MospfPrintGroupMembershipLSAList(node);
    }
    // Print out stats if user specifies it.
    if (mospf->showStat == TRUE)
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "Group Membership LSAs Generated = %d",
            mospf->stats.numGroupMembershipLSAGenerated);
        IO_PrintStat(
            node,
            "Network",
            "MOSPF",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Group Membership LSAs Flushed = %d",
            mospf->stats.numGroupMembershipLSAFlushed);
        IO_PrintStat(
            node,
            "Network",
            "MOSPF",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Group Joins = %d",
            mospf->stats.numGroupJoin);
        IO_PrintStat(
            node,
            "Network",
            "MOSPF",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Group Leaves = %d",
            mospf->stats.numGroupLeave);
        IO_PrintStat(
            node,
            "Network",
            "MOSPF",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Multicast Packets Generated = %d",
            mospf->stats.numOfMulticastPacketGenerated);
        IO_PrintStat(
            node,
            "Network",
            "MOSPF",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Multicast Packets Discarded = %d",
            mospf->stats.numOfMulticastPacketDiscard);
        IO_PrintStat(
            node,
            "Network",
            "MOSPF",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Multicast Packets Received = %d",
            mospf->stats.numOfMulticastPacketReceived);
        IO_PrintStat(
            node,
            "Network",
            "MOSPF",
            ANY_DEST,
            -1, // instance Id,
            buf);

        sprintf(buf, "Multicast Packets Forwarded = %d",
            mospf->stats.numOfMulticastPacketForwarded);
        IO_PrintStat(
            node,
            "Network",
            "MOSPF",
            ANY_DEST,
            -1, // instance Id,
            buf);
    }
}

