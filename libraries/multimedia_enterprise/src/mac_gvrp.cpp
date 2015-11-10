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


#include "api.h"
#include "partition.h"

#include "mac_switch.h"
#include "mac_garp.h"
#include "mac_gvrp.h"


// Standard GVRP group address is 01-80-c2-00-00-21
const unsigned char gvrpGroupAddress[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x21};

// -------------------------------------------------------------------------
// GVRP Library functions

// NAME:        SwitchGvrp_GetGvrpData
//
// PURPOSE:     Return gvrp data for a switch.
//
// PARAMETERS:  This switch
//
// RETURN:      Pointer to gvrp data.

SwitchGvrp* SwitchGvrp_GetGvrpData(
    MacSwitch* sw)
{
    return sw->gvrp;
}


// NAME:        SwitchGvrp_EnablePort
//
// PURPOSE:     Enable the GarpGid port for this Garp
//              application, at this switch port.
//
// PARAMETERS:  Node data.
//              This switch.
//              Switch port enabled.
//
// RETURN:      None.

void SwitchGvrp_EnablePort(
    Node* node,
    MacSwitch* sw,
    SwitchPort* thePort)
{
    SwitchGvrp* gvrp = (SwitchGvrp*) SwitchGvrp_GetGvrpData(sw);
    GarpGid* portGid = NULL;

    if (GarpGid_FindPort(gvrp->garp.gid, thePort->portNumber, &portGid))
    {
        portGid->isEnabled = TRUE;
    }
}


// NAME:        SwitchGvrp_DisablePort
//
// PURPOSE:     Disable the GarpGid port exists for this Garp
//              application, at this switch port.
//
// PARAMETERS:  Node data.
//              This switch.
//              Switch port disabled.
//
// RETURN:      None.

void SwitchGvrp_DisablePort(
    Node* node,
    MacSwitch* sw,
    SwitchPort* thePort)
{
    SwitchGvrp* gvrp = (SwitchGvrp*) SwitchGvrp_GetGvrpData(sw);
    GarpGid* portGid = NULL;

    if (GarpGid_FindPort(gvrp->garp.gid, thePort->portNumber, &portGid))
    {
        portGid->isEnabled = FALSE;
    }
}


// NAME:        SwitchGvrp_GetPtrToPortStats
//
// PURPOSE:     Get stat pointer for given port.
//
// PARAMETERS:  Switch data.
//              SwitchGvrp data.
//              Port number
//
// RETURN:      Stat pointer for given port.

static
GvrpStat* SwitchGvrp_GetPtrToPortStats(
    MacSwitch* sw,
    SwitchGvrp* gvrp,
    unsigned short portNumber)
{
    unsigned i;
    char errString[MAX_STRING_LENGTH];
    GvrpStat* portStat = (GvrpStat*) gvrp->stats;
    SwitchPort* portPtr = Switch_GetPortDataFromPortNumber(
                            sw, portNumber);

    for (i = 0; i < gvrp->numPorts; i++)
    {
        if (MAC_IsIdenticalMac802Address(&portStat->portAddr,
                                         &portPtr->portAddr))
        {
            return portStat;
        }
        portStat++;
    }

    // Stat variable not found for this port.
    //
    sprintf(errString,
        "SwitchGvrp_GetPtrToPortStats: "
        "Stat variable not found for port %u in switch %d\n",
        portNumber, sw->swNumber);
    ERROR_ReportError(errString);

    return NULL;
}


// NAME:        SwitchGvrp_UpdatePortStats
//
// PURPOSE:     Update statistics for given port.
//
// PARAMETERS:  Switch data.
//              SwitchGvrp data.
//              Port number.
//              Event of a message.
//
// RETURN:      None.

static
void SwitchGvrp_UpdatePortStats(
    MacSwitch* sw,
    SwitchGvrp* gvrp,
    unsigned short portNumber,
    GarpGidEvent event)
{
    GvrpStat* portStat = SwitchGvrp_GetPtrToPortStats(sw, gvrp, portNumber);

    switch (event)
    {
        case GARP_GID_RX_JOIN_EMPTY:
            (portStat->joinEmptyReceived)++;
            break;
        case GARP_GID_RX_JOIN_IN:
            (portStat->joinInReceived)++;
            break;
        case GARP_GID_RX_LEAVE_IN:
            (portStat->leaveInReceived)++;
            break;
        case GARP_GID_RX_LEAVE_EMPTY:
            (portStat->leaveEmptyReceived)++;
            break;
        case GARP_GID_RX_LEAVE_ALL:
            (portStat->leaveAllReceived)++;
            break;
        case GARP_GID_TX_JOIN_EMPTY:
            (portStat->joinEmptyTransmitted)++;
            break;
        case GARP_GID_TX_JOIN_IN:
            (portStat->joinInTransmitted)++;
            break;
        case GARP_GID_TX_LEAVE_IN:
            (portStat->leaveInTransmitted)++;
            break;
        case GARP_GID_TX_LEAVE_EMPTY:
            (portStat->leaveEmptyTransmitted)++;
            break;
        case GARP_GID_TX_LEAVE_ALL:
            (portStat->leaveAllTransmitted)++;
            break;
        case GARP_GID_RX_EMPTY:
        case GARP_GID_TX_EMPTY:
        default:
            break;
    }
}


// -------------------------------------------------------------------------
// GVD : GARP VLAN Database

// NAME:        GvrpGvd_CreateGvd
//
// PURPOSE:     Creates a new instance of gvd.
//
// PARAMETERS:  Maximum vlans to handle.
//
// RETURN:      TRUE, if gvd created with a pointer to gvd.

static
BOOL GvrpGvd_CreateGvd(
    int vlansMax,
    VlanId** gvd)
{
    VlanId* aGvd = (VlanId*) MEM_malloc(sizeof(VlanId) * vlansMax);
    memset(aGvd, 0, sizeof(VlanId) * vlansMax);
    *gvd = aGvd;

    return TRUE;
}


// NAME:        GvrpGvd_DestroyGvd
//
// PURPOSE:     Destroys the instance of gvd.
//
// PARAMETERS:  Gvd to free.
//
// RETURN:      None.

static
void GvrpGvd_DestroyGvd(
    VlanId* gvd)
{
    MEM_free(gvd);
}


// NAME:        GvrpGvd_FindEntry
//
// PURPOSE:     Find an entry in gvd.
//
// PARAMETERS:  SwitchGvrp data.
//              Entry to find.
//
// RETURN:      TRUE, if entry is found.
//              FALSE, otherwise.

static
BOOL GvrpGvd_FindEntry(
    SwitchGvrp* gvrp,
    VlanId key,
    unsigned* foundAtIndex)
{
    unsigned i;
    VlanId* aGvd = (VlanId*) gvrp->gvd;

    for (i = 0; i < gvrp->vlansMax; i++)
    {
        if (*aGvd == key)
        {
            *foundAtIndex = i;
            return TRUE;
        }
        aGvd++;
    }

    return FALSE;
}


// NAME:        GvrpGvd_CreateEntry
//
// PURPOSE:     Create a new entry in gvd.
//
// PARAMETERS:  SwitchGvrp data.
//              Entry to create.
//
// RETURN:      TRUE, if created.
//              FALSE, otherwise.

static
BOOL GvrpGvd_CreateEntry(
    SwitchGvrp* gvrp,
    VlanId key,
    unsigned* createdAtIndex)
{
    unsigned i;
    VlanId* aGvd = (VlanId*) gvrp->gvd;

    for (i = 0; i < gvrp->vlansMax; i++)
    {
        if (*aGvd == 0)
        {
            *aGvd = key;
            *createdAtIndex = i;

            // Increase index if it is first entry in
            // database or a new entey.
            // Note: No decrement operation is done on
            //      lastGidIndex, as all possible active
            //      machines lie within this range.
            //
            if (gvrp->garp.lastGidIndex == 0 ||
                (gvrp->garp.lastGidIndex - 1) < *createdAtIndex)
            {
                gvrp->garp.lastGidIndex++;
            }
            return TRUE;
        }
        aGvd++;
    }

    return FALSE;
}


// NAME:        GvrpGvd_DeleteEntry
//
// PURPOSE:     Delete an entry from gvd.
//
// PARAMETERS:  SwitchGvrp data.
//              Location of entry to delete.
//
// RETURN:      TRUE, if deleted.
//              FALSE, otherwise.

static
BOOL GvrpGvd_DeleteEntry(
    SwitchGvrp* gvrp,
    unsigned deleteAtIndex)
{
    VlanId* gvd = (VlanId*) gvrp->gvd;

    if (deleteAtIndex < gvrp->vlansMax)
    {
        *((VlanId*) (gvd + deleteAtIndex)) = 0;
        return TRUE;
    }

    return FALSE;
}


// NAME:        GvrpGvd_GetKey
//
// PURPOSE:     Get an entry from gvd at a given index.
//
// PARAMETERS:  SwitchGvrp data.
//              Index of entry.
//
// RETURN:      TRUE, if find with proper key.
//              FALSE, otherwise.

static
BOOL GvrpGvd_GetKey(
    SwitchGvrp* gvrp,
    unsigned index,
    VlanId* key)
{
    VlanId* gvd = (VlanId*) gvrp->gvd;

    if (index < gvrp->vlansMax)
    {
        *key = *((VlanId*)gvd + index);

        if (*key != 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}


// -------------------------------------------------------------------------
// VLAN database interface

// NAME:        GvrpVfdb_Forward
//
// PURPOSE:     Add a port in member set for given vlan in
//              active filter database.
//
// PARAMETERS:  Node data.
//              Port to add.
//              Vlan id.
//
// RETURN:      None.

static
void GvrpVfdb_Forward(
    Node* node,
    unsigned short portNumber,
    VlanId vlanId)
{
    MacSwitch* sw = (MacSwitch*) Switch_GetSwitchData(node);
    SwitchVlanDbItem* dbItem = NULL;
    unsigned short* aPortNumber = NULL;

    // Check if database already maintained for this vlan. Create
    // a fresh entry if none found for this vlan.

    dbItem = SwitchVlan_GetDbItem(sw, vlanId);
    if (dbItem)
    {
        ERROR_Assert(dbItem->dynamicMemberSet,
            "GvrpVfdb_Forward: Dynamic member set not initialized.\n");

        if (!SwitchVlan_IsPortInMemberList(
                dbItem->dynamicMemberSet, portNumber))
        {
            aPortNumber = (unsigned short*) MEM_malloc(sizeof(short));
            *aPortNumber = portNumber;
            ListInsert(
                node, dbItem->dynamicMemberSet, 0, (void*) aPortNumber);
        }
    }
    else
    {
        SwitchVlanDbItem* aDbItem = NULL;

        // Entry does not exist for this vlanId. Create the
        // entry and add the vlan to dynamic member set.
        //
        aDbItem = (SwitchVlanDbItem*) MEM_malloc(sizeof(SwitchVlanDbItem));

        aDbItem->vlanId = vlanId;
        switch (sw->learningType)
        {
            case SWITCH_LEARNING_SHARED:
                aDbItem->currentFid = SWITCH_FID_DEFAULT;
                break;

            case SWITCH_LEARNING_INDEPENDENT:
                aDbItem->currentFid = vlanId;
                break;

            case SWITCH_LEARNING_COMBINED:
                aDbItem->currentFid =
                    SwitchDb_GetFidForGivenVlan(sw, vlanId);
                if (aDbItem->currentFid == SWITCH_FID_UNKNOWN)
                {
                    aDbItem->currentFid = SWITCH_FID_DEFAULT;
                }
                break;

            default:
                ERROR_ReportError(
                    "GvrpVfdb_Forward: Unknown VLAN learning type.\n");
                break;
        }

        ListInit(node, &aDbItem->memberSet);
        ListInit(node, &aDbItem->untaggedSet);
        ListInit(node, &aDbItem->dynamicMemberSet);

        // Insert port in dynamicMemberSet.
        //
        aPortNumber = (unsigned short*) MEM_malloc(sizeof(short));
        *aPortNumber = portNumber;
        ListInsert(node, aDbItem->dynamicMemberSet, 0, (void*) aPortNumber);

        // Insert this vlan entry into database.
        //
        ListInsert(node, sw->vlanDbList, 0, (void*) aDbItem);
    }
}


// NAME:        GvrpVfdb_Filter
//
// PURPOSE:     Filter the port from member set for given vlan
//              from active filter database.
//
// PARAMETERS:  Node data.
//              Port to add.
//              Vlan id.
//
// RETURN:      None.

static
void GvrpVfdb_Filter(
    Node* node,
    unsigned short portNumber,
    VlanId vlanId)
{
    ListItem* dynamicItem = NULL;
    char errString[MAX_STRING_LENGTH];
    MacSwitch* sw = (MacSwitch*) Switch_GetSwitchData(node);
    SwitchVlanDbItem* dbItem = SwitchVlan_GetDbItem(sw, vlanId);

    if (!dbItem)
    {
        sprintf(errString,
            "GvrpVfdb_Filter: Db entry not found for vlan %u.\n",
            vlanId);
        ERROR_ReportError(errString);
    }

    dynamicItem = dbItem->dynamicMemberSet->first;
    while (dynamicItem)
    {
        if (*((unsigned short*)(dynamicItem->data)) == portNumber)
        {
            // Remove the port from dynamic member set.
            //
            ListGet(node, dbItem->dynamicMemberSet,
                dynamicItem, TRUE, FALSE);
            break;
        }
        dynamicItem = dynamicItem->next;
    }
}


// -------------------------------------------------------------------------
// Gvrp Trace
//
#define GVRP_TRACE_DEFAULT               FALSE
#define GVRP_TRACE_GID_FLAGS             1
#define GVRP_TRACE_GID_STATE             0
#define GVRP_TRACE_ATTRIB_STATE_FLAGS    0
#define GVRP_TRACE_MESSAGE_DETAILS       1
#define GVRP_TRACE_COMMENTS              1


// NAME:        GvrpTrace_GetMsgEvent
//
// PURPOSE:     Write message event in a temporary buffer.
//
// PARAMETERS:  Event to write.
//
// RETURN:      None.

static
void GvrpTrace_GetMsgEvent(
    GarpGidEvent event,
    char* const eventStr)
{
    switch (event)
    {
        case GARP_GID_NULL:
            strcpy(eventStr, "Null");
            break;

        case GARP_GID_RX_LEAVE_ALL:
            strcpy(eventStr, "RLAll");
            break;

        case GARP_GID_RX_JOIN_EMPTY:
            strcpy(eventStr, "RJEmt");
            break;

        case GARP_GID_RX_JOIN_IN:
            strcpy(eventStr, "RJIn");
            break;

        case GARP_GID_RX_LEAVE_EMPTY:
            strcpy(eventStr, "RLEmt");
            break;

        case GARP_GID_RX_LEAVE_IN:
            strcpy(eventStr, "RLIn");
            break;

        case GARP_GID_RX_EMPTY:
            strcpy(eventStr, "REmt");
            break;

        case GARP_GID_RX_LEAVE_ALL_RANGE:
            strcpy(eventStr, "RLARg");
            break;

        case GARP_GID_TX_LEAVE_ALL:
            strcpy(eventStr, "TLAll");
            break;

        case GARP_GID_TX_JOIN_EMPTY:
            strcpy(eventStr, "TJEmt");
            break;

        case GARP_GID_TX_JOIN_IN:
            strcpy(eventStr, "TJIn");
            break;

        case GARP_GID_TX_LEAVE_EMPTY:
            strcpy(eventStr, "TLEmt");
            break;

        case GARP_GID_TX_LEAVE_IN:
            strcpy(eventStr, "TLIn");
            break;

        case GARP_GID_TX_EMPTY:
            strcpy(eventStr, "TEmt");
            break;

        case GARP_GID_TX_LEAVE_ALL_RANGE:
            strcpy(eventStr, "TLARg");
            break;

        default:
            strcpy(eventStr, "???");
            break;
    }
}


// NAME:        GvrpTrace_GetMsgAttribute
//
// PURPOSE:     Write message attribute in a temporary buffer.
//
// PARAMETERS:  Attribute to write.
//
// RETURN:      None.

static
void GvrpTrace_GetMsgAttribute(
    GvrpAttributeType attrib,
    char* const attributeStr)
{
    switch (attrib)
    {
        case GVRP_VLAN_ATTRIBUTE:
            strcpy(attributeStr, "Vlan");
            break;

        default:
            strcpy(attributeStr, "???");
            break;
    }
}


// NAME:        GvrpTrace_GetApplicantState
//
// PURPOSE:     Write applicant state in a temporary buffer.
//
// PARAMETERS:  Applicant state to write.
//
// RETURN:      None.

static
void GvrpTrace_GetApplicantState(
    GarpGid_ApplicantState state,
    char* const stateStr)
{
    switch (state)
    {
        case GARP_GID_VeryAnxious:
            strcpy(stateStr, "VA");
            break;
        case GARP_GID_Anxious:
            strcpy(stateStr, "A");
            break;
        case GARP_GID_Quiet:
            strcpy(stateStr, "Q");
            break;
        case GARP_GID_Leaving:
            strcpy(stateStr, "L");
            break;
        default:
            strcpy(stateStr, "??");
            break;
    }
}


// NAME:        GvrpTrace_GetApplicantMgt
//
// PURPOSE:     Write applicant management state in a
//              temporary buffer.
//
// PARAMETERS:  Applicant management state to write.
//
// RETURN:      None.

static
void GvrpTrace_GetApplicantMgt(
    GarpGid_ApplicantMgt mgt,
    char* const mgtStr)
{
    switch (mgt)
    {
        case GARP_GID_Normal:
            strcpy(mgtStr, "Normal");
            break;
        case GARP_GID_NoProtocol:
            strcpy(mgtStr, "NoProto");
            break;
        default:
            strcpy(mgtStr, "???");
            break;
    }
}


// NAME:        GvrpTrace_GetRegistrarState
//
// PURPOSE:     Write registrar state in a temporary buffer.
//
// PARAMETERS:  Registrar state to write.
//
// RETURN:      None.

static
void GvrpTrace_GetRegistrarState(
    GarpGid_RegistrarState state,
    char* const stateStr)
{
    switch (state)
    {
        case GARP_GID_In:
            strcpy(stateStr, "In");
            break;
        case GARP_GID_Leave:
            strcpy(stateStr, "Lv");
            break;
        case GARP_GID_Empty:
            strcpy(stateStr, "Mt");
            break;
        default:
            strcpy(stateStr, "??");
            break;
    }
}


// NAME:        GvrpTrace_GetRegistrarMgt
//
// PURPOSE:     Write registrar management state in
//              a temporary buffer.
//
// PARAMETERS:  Registrar management state to write.
//
// RETURN:      None.

static
void GvrpTrace_GetRegistrarMgt(
    GarpGid_RegistrarMgt mgt,
    char* const mgtStr)
{
    switch (mgt)
    {
        case GARP_GID_NormalRegistration:
            strcpy(mgtStr, "NomReg");
            break;
        case GARP_GID_RegistrationFixed:
            strcpy(mgtStr, "FixReg");
            break;
        case GARP_GID_RegistrationForbidden:
            strcpy(mgtStr, "FbdReg");
            break;
        default:
            strcpy(mgtStr, "???");
            break;
    }
}


// NAME:        GvrpTrace_WriteAttributeState
//
// PURPOSE:     Write applicant and registrar states for a
//              vlan in a trace file.
//
// PARAMETERS:  Node data.
//              Switch data.
//              Port gid.
//              Index in state machine for the vlan.
//
// RETURN:      None.

static
void GvrpTrace_WriteAttributeState(
    Node* node,
    MacSwitch* sw,
    GarpGid* portGid,
    unsigned int gidIndex,
    FILE* fp)
{
    GarpGidStates state;
    char applicantStateStr[20];
    char registrarStateStr[20];
    char applicantMgtStr[20];
    char registrarMgtStr[20];

    GarpGidTt_States(&portGid->machines[gidIndex], &state);
    GvrpTrace_GetApplicantState(
        (GarpGid_ApplicantState) state.applicantState, applicantStateStr);
    GvrpTrace_GetRegistrarState(
        (GarpGid_RegistrarState) state.registrarState, registrarStateStr);
    GvrpTrace_GetRegistrarMgt(
        (GarpGid_RegistrarMgt) state.registrarMgt, registrarMgtStr);
    GvrpTrace_GetApplicantMgt(
        (GarpGid_ApplicantMgt) state.applicantMgt, applicantMgtStr);

    fprintf(fp, " [%2s : %7s :: %2s : %6s]",
        applicantStateStr, applicantMgtStr,
        registrarStateStr, registrarMgtStr);
}


// NAME:        GvrpTrace_WriteAllAttributeStates
//
// PURPOSE:     Write applicant and registrar states for all
//              vlans at a port in a trace file.
//
// PARAMETERS:  Node data.
//              Switch data.
//              Port gid.
//
// RETURN:      None.

static
void GvrpTrace_WriteAllAttributeStates(
    Node* node,
    MacSwitch* sw,
    GarpGid* portGid,
    FILE* fp)
{
    unsigned gidIndex = 0;
    VlanId key;

    fprintf(fp,
        "       Attributes for Sw %d on port %d\n",
        sw->swNumber, portGid->portNumber);

    for (gidIndex = 0; gidIndex < portGid->garp->lastGidIndex;
            gidIndex++)
    {
        if (GvrpGvd_GetKey(sw->gvrp, gidIndex, &key))
        {
            fprintf(fp, "          vlan %d ", key);
            GvrpTrace_WriteAttributeState(
                node, sw, portGid, gidIndex, fp);
            fprintf(fp, "\n");
        }
    }
    fprintf(fp, "\n");
}


// NAME:        GvrpTrace_WritePortState
//
// PURPOSE:     Write state of a port in a trace file.
//
// PARAMETERS:  Node data.
//              Switch data.
//              Port gid.
//
// RETURN:      None.

static
void GvrpTrace_WritePortState(
    Node* node,
    MacSwitch* sw,
    GarpGid* portGid,
    FILE* fp)
{
    fprintf(fp,
        "Port %-2d on switch %-4d --- ",
        portGid->portNumber, sw->swNumber);

    //  Write port's state
    //  [ IsEnabled : isConnected : isPointToPoint]
    //
    fprintf(fp, "["
        "Enabled  %5s :: Connected  %5s :: Port P2P  %5s]\n",
        portGid->isEnabled ? "TRUE" : "FALSE",
        portGid->isConnected ? "TRUE" : "FALSE",
        portGid->isPointToPoint ? "TRUE" : "FALSE");
}


// NAME:        GvrpTrace_WritePortGidFlags
//
// PURPOSE:     Write different flags of a port in a trace file.
//
// PARAMETERS:  Node data.
//              Switch data.
//              Port gid.
//
// RETURN:      None.

static
void GvrpTrace_WritePortGidFlags(
    Node* node,
    MacSwitch* sw,
    GarpGid* portGid,
    FILE* fp)
{
    //  Write timer flag states
    //  [ StartJT:JTRunning StartLT:LTRunning
    //           TxNow:TxNowScheduled holdTx:TxPending]
    //
    fprintf(fp, "[%3s:%3s  %3s:%3s  %3s:%3s  %3s:%3s]\n",
        portGid->canStartJoinTimer ? "sJT" : "",
        portGid->joinTimerRunning ? "JTr" : "",
        portGid->canStartLeaveTimer ? "sLT" : "",
        portGid->leaveTimerRunning ? "LTr" : "",
        portGid->canScheduleTxNow ? "TXn" : "",
        portGid->txNowScheduled ? "TXs" : "",
        portGid->holdTx ? "hTX" : "",
        portGid->txPending ? "TXp" : "");
}


// NAME:        Gvrp_TraceWriteCommonHdrFields
//
// PURPOSE:     Write common headers of a message in a trace file.
//
// PARAMETERS:  Node data.
//              Switch data.
//              Port gid.
//
// RETURN:      None.

static
void Gvrp_TraceWriteCommonHdrFields(
    Node* node,
    MacSwitch* sw,
    GarpGid* portGid,
    GvrpAttributeType attribute,
    char* const flag,
    UInt32 serialNo,
    FILE* const fp)
{
    float simTime = node->getNodeTime()/((float)SECOND);
    char attribTypeStr[20];

    fprintf(fp, "%4u) %4u %2u %c ",
        serialNo,
        sw->swNumber,
        portGid->portNumber,
        *flag);

    GvrpTrace_GetMsgAttribute(attribute, attribTypeStr),

    fprintf(fp, "%s %.4f --- ",
        attribTypeStr, simTime);
}


// NAME:        SwitchGvrp_Trace
//
// PURPOSE:     Creates the trace file for gvrp for incoming and
//              outgoing messages, different flags at a port, vlan
//              attributes at a ports etc.
//
// PARAMETERS:  Node data.
//              Port gid.
//              Message pointer.
//
// RETURN:      None.

void SwitchGvrp_Trace(
    Node* node,
    GarpGid* portGid,
    Message* msg,
    const char* flag)
{
    // static UInt32 count = 0;
    // MacSwitch* sw = Switch_GetSwitchData(node);
    // FILE* fp = NULL;

    // To be implemented.
    return;
   /* if (!sw->gvrpTrace)
    {
        return;
    }

    if (count == 0)
    {
        fp = fopen("gvrp.trace", "w");
        ERROR_Assert(fp != NULL,
            "SwitchGvrp_Trace: file initial open error.\n");
    }
    else
    {
        fp = fopen("gvrp.trace", "a");
        ERROR_Assert(fp != NULL,
            "SwitchGvrp_Trace: file append error.\n");
    }


    // Write GarpGid port state
    if (!strcmp(flag, "PortState"))
    {
        if (GVRP_TRACE_GID_STATE)
        {
            GvrpTrace_WritePortState(node, sw, portGid, fp);
        }
        fclose(fp);
        return;
    }

    // Write GarpGid flags
    if (!strcmp(flag, "GidFlags"))
    {
        if (GVRP_TRACE_GID_FLAGS)
        {
            fprintf(fp, "Port %2d on switch %2d --- ",
                portGid->portNumber, sw->swNumber);
            GvrpTrace_WritePortGidFlags(node, sw, portGid, fp);
        }
        fclose(fp);
        return;
    }

    // Write Attribute states
    if (!strcmp(flag, "AttribState"))
    {
        if (GVRP_TRACE_ATTRIB_STATE_FLAGS)
        {
            GvrpTrace_WriteAllAttributeStates(node, sw, portGid, fp);
        }
        fclose(fp);
        return;
    }

    // Write any comment that is passed as parameter
    if (strcmp(flag, "Send") && strcmp(flag, "Receive"))
    {
        if (GVRP_TRACE_COMMENTS)
        {
            float simTime = node->getNodeTime()/((float)SECOND);
            fprintf(fp, "Switch %-4d Port %-2d %.4f  --- %s\n",
                sw->swNumber,
                portGid != NULL ? portGid->portNumber : 0,
                simTime,
                flag);
        }
        fclose(fp);
        return;
    }

    count++;

    if (!strcmp(flag, "Send")
        || !strcmp(flag, "Receive"))
    {
        GarpGidEvent event;
        GvrpGvfMsg gvfMsg;
        char attribEventStr[10];
        char* garpAttributes = (char*) MESSAGE_ReturnPacket(msg);

        // Pass protocol Id field
        garpAttributes = garpAttributes + (sizeof(char) * 2);
        gvfMsg.attribute = (GvrpAttributeType)(*garpAttributes);
        garpAttributes++;

        while (gvfMsg.attribute != GARP_END_OF_PDU)
        {
            Gvrp_TraceWriteCommonHdrFields(
                node, sw, portGid, gvfMsg.attribute, flag, count, fp);
            GvrpTrace_WritePortGidFlags(node, sw, portGid, fp);


            gvfMsg.length = *garpAttributes;
            garpAttributes++;
            while (gvfMsg.length != 0)
            {
                event = (GarpGidEvent)(*garpAttributes);
                Garp_DecodeReceivedEvent(event, &gvfMsg.event);
                garpAttributes++;

                switch (gvfMsg.event)
                {
                    case GARP_GID_RX_LEAVE_ALL:
                        gvfMsg.key1 = SWITCH_VLAN_ID_INVALID;
                        break;

                    case GARP_GID_RX_JOIN_EMPTY:
                    case GARP_GID_RX_JOIN_IN:
                    case GARP_GID_RX_LEAVE_EMPTY:
                    case GARP_GID_RX_LEAVE_IN:
                        gvfMsg.key1 = (unsigned short) (*garpAttributes * GVRP_BASE_VALUE);
                        garpAttributes++;
                        gvfMsg.key1 += ((unsigned short) (*garpAttributes));
                        garpAttributes++;
                        break;

                    default:
                        gvfMsg.key1 = SWITCH_VLAN_ID_INVALID;
                        break;
                } // switch

                if (GVRP_TRACE_MESSAGE_DETAILS)
                {
                    if (!strcmp(flag, "Receive"))
                        event = gvfMsg.event;
                    GvrpTrace_GetMsgEvent(event, attribEventStr);
                    fprintf(fp, "                                  "
                        "[%2d : %5s]", gvfMsg.key1, attribEventStr);
                    fprintf(fp, "\n");
                }

                gvfMsg.length = *garpAttributes;
                garpAttributes++;
            }
            gvfMsg.attribute = (GvrpAttributeType)(*garpAttributes);
            garpAttributes++;
        }
    }

    fclose(fp);*/
}


// NAME:        SwitchGvrp_TraceInit
//
// PURPOSE:     Initialize the gvrp trace for a switch.
//
// PARAMETERS:  Node data.
//              Switch data.
//
// RETURN:      None.

static
void SwitchGvrp_TraceInit(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    // char yesOrNo[MAX_STRING_LENGTH];
    // BOOL retVal = FALSE;

    // To be implemented.
    return;
    // Initialize trace values for the node
    // <TraceType> is one of
    //      NO    the default if nothing is specified
    //      YES   an ascii format
    // Format is: GVRP-TRACE <TraceType>

    /*IO_ReadString(
         node->nodeId,
         ANY_ADDRESS,
         nodeInput,
         "GVRP-TRACE",
         &retVal,
         yesOrNo);

    sw->gvrpTrace = GVRP_TRACE_DEFAULT;
    if (retVal == TRUE)
    {
        if (!strcmp(yesOrNo, "NO"))
        {
            sw->gvrpTrace = FALSE;
        }
        else if (!strcmp(yesOrNo, "YES"))
        {
            sw->gvrpTrace = TRUE;
        }
        else
        {
            ERROR_ReportError(
                "SwitchGvrp_TraceInit: "
                "Unknown value of GVRP-TRACE in configuration file.\n"
                "Expecting YES or NO\n");
        }
    }*/
}


// -------------------------------------------------------------------------
// Port Management

// NAME:        SwitchGvrp_AddNewPort
//
// PURPOSE:     Query the permanent database for static VLAN entries
//              and set applicant and registrar state machines for
//              those entries for the given port.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port number.
//
// RETURN:      None.

void SwitchGvrp_AddNewPort(
    Node* node,
    void* garpPtr,
    unsigned short portNumber)
{
    MacSwitch* sw = (MacSwitch*) Switch_GetSwitchData(node);
    SwitchGvrp* gvrp = (SwitchGvrp*) SwitchGvrp_GetGvrpData(sw);
    SwitchPort* port = Switch_GetPortDataFromPortNumber(sw, portNumber);

    VlanId vlanId = SWITCH_VLAN_ID_INVALID;
    unsigned int gvdIndex = 0;
    GarpGid* portGid = NULL;

    if (!GarpGid_FindPort(gvrp->garp.gid, portNumber, &portGid))
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString,
            "SwitchGvrp_AddNewPort: Port %d not present in ring.\n",
            portNumber);
        ERROR_ReportError(errString);
    }


    // Find the fixed registration entries that are
    // configured at each port via user input.
    //
    vlanId = SwitchVlan_GetStaticMembership(sw, portNumber, TRUE, vlanId);

    if (portGid != NULL && vlanId != SWITCH_VLAN_ID_INVALID)
    {
        do
        {
            // Special VLANs reserved for this implementation.
            // No need to maintain GID machines for these VLANs..
            //
            if (vlanId == SWITCH_VLAN_ID_STP ||
                vlanId == SWITCH_VLAN_ID_GVRP)
            {
                continue;
            }

            if (!GvrpGvd_FindEntry(gvrp, vlanId, &gvdIndex))
            {
                (void) GvrpGvd_CreateEntry(
                           gvrp, vlanId, &gvdIndex);
            }

            GarpGid_ManageAttribute(
                node, portGid, gvdIndex, GARP_GID_FIX_REGISTRATION);

            // Add vlan in dynamic member set.
            //
            GvrpVfdb_Forward(node, portNumber, vlanId);

        } while (( vlanId = SwitchVlan_GetStaticMembership(
                                sw, portNumber, FALSE, vlanId))
                  != SWITCH_VLAN_ID_INVALID);
    }

    // Get and, if not existing, create GID applicant and registrar
    // machines for the port VLAN id configured at this port.
    // Add pVid to dynamic member set.
    //
    if (!GvrpGvd_FindEntry(gvrp, port->pVid, &gvdIndex))
    {
        (void) GvrpGvd_CreateEntry(gvrp, port->pVid, &gvdIndex);
    }

    GarpGid_ManageAttribute(
        node, portGid, gvdIndex, GARP_GID_FIX_REGISTRATION);
    GvrpVfdb_Forward(node, portNumber, port->pVid);
}


// NAME:        SwitchGvrp_RemovePort
//
// PURPOSE:     Removes entries from database for the given port.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port number.
//
// RETURN:      None.

void SwitchGvrp_RemovePort(
    Node* node,
    void* garpPtr,
    unsigned short portNumber)
{
    MacSwitch* sw = (MacSwitch*) Switch_GetSwitchData(node);

    VlanId vlanId;
    ListItem* listItem = NULL;
    ListItem* memberSetItem = NULL;
    SwitchVlanDbItem* dbItem = NULL;

    listItem = sw->vlanDbList->first;
    while (listItem)
    {
        dbItem = (SwitchVlanDbItem*) listItem->data;

        // Check if this port inserted any dynamic entry.
        //
        memberSetItem = dbItem->dynamicMemberSet->first;
        while (memberSetItem)
        {
            if (*((unsigned short*) (memberSetItem->data)) ==
                portNumber)
            {
                ListItem* itemToFree = NULL;
                vlanId = dbItem->vlanId;

                itemToFree = memberSetItem;
                memberSetItem = memberSetItem->next;

                // Remove learnt entry from dynamic member set.
                //
                ListGet(node, dbItem->dynamicMemberSet,
                    itemToFree, TRUE, FALSE);
            }
            else
            {
                memberSetItem = memberSetItem->next;
            }
        }
        listItem = listItem->next;
    }
}


// -------------------------------------------------------------------------
// GARP VLAN Registration  JOIN and LEAVE Indications

// NAME:        SwitchGvrp_JoinIndication
//
// PURPOSE:     Indicate the join of a vlan at this port. Add the
//              port in member set for this vlan.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port GarpGid.
//              GarpGid index.
//
// RETURN:      None.

static
void SwitchGvrp_JoinIndication(
    Node* node,
    void* garpPtr,
    void* portGidPtr,
    unsigned gidIndex)
{
    MacSwitch* sw = (MacSwitch*) Switch_GetSwitchData(node);
    SwitchGvrp* gvrp = (SwitchGvrp*) SwitchGvrp_GetGvrpData(sw);
    GarpGid* portGid = (GarpGid*) portGidPtr;

    VlanId key = SWITCH_VLAN_ID_INVALID;
    char trcStr[MAX_STRING_LENGTH];

    if (GvrpGvd_GetKey(gvrp, gidIndex, &key))
    {
        GvrpVfdb_Forward(node, portGid->portNumber, key);

        sprintf(trcStr, "Vlan %d added to database.", key);
        SwitchGvrp_Trace(node, portGid, NULL, trcStr);
    }
}


// NAME:        SwitchGvrp_JoinLeavePropagated
//
// PURPOSE:     Propagate the join or leave indication.
//
// NOTE:        Nothing to be done since, unlike GMR with its Forward
//              All Unregistered Port mode, a join indication on one port
//              does not cause filtering to be instantiated on another.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port GarpGid.
//              GarpGid index.
//
// RETURN:      None.

static
void SwitchGvrp_JoinLeavePropagated(
    Node* node,
    void* garpPtr,
    void* portGidPtr,
    unsigned gidIndex)
{
}


// NAME:        SwitchGvrp_LeaveIndication
//
// PURPOSE:     Indicate the leave of a vlan from this port. Remove the
//              port from member set for this vlan.
//
// PARAMETERS:  Node data.
//              Garp data.
//              Port GarpGid.
//              GarpGid index.
//
// RETURN:      None.

static
void SwitchGvrp_LeaveIndication(
    Node* node,
    void* garpPtr,
    void* portGidPtr,
    unsigned leavingGidIndex)
{
    GarpGid* portGid = (GarpGid*) portGidPtr;
    MacSwitch* sw = (MacSwitch*) Switch_GetSwitchData(node);
    SwitchGvrp* gvrp = (SwitchGvrp*) SwitchGvrp_GetGvrpData(sw);

    VlanId key = SWITCH_VLAN_ID_INVALID;
    char trcStr[MAX_STRING_LENGTH];

    if (GvrpGvd_GetKey(gvrp, leavingGidIndex, &key))
    {
        GvrpVfdb_Filter(node, portGid->portNumber, key);

        sprintf(trcStr, "Vlan %d leaving from database.", key);
        SwitchGvrp_Trace(node, portGid, NULL, trcStr);
    }
}


// -------------------------------------------------------------------------
// Receive message processing

// NAME:        SwitchGvrp_DbFull
//
// PURPOSE:     Alert function indicating that there are
//              registrations for more VLANs than can be accepted.
//
// PARAMETERS:  SwitchGvrp data.
//              Port GarpGid.
//
// RETURN:      None.

static
void SwitchGvrp_DbFull(
    SwitchGvrp* gvrp,
    GarpGid* portGid,
    VlanId vlanId)
{
    char errString[MAX_STRING_LENGTH];

    sprintf(errString,
        "SwitchGvrp_DbFull: "
        "The GVRP VLAN database is sized for max %u entries.\n"
        "Could not include VLAN %u in database.\n"
        "Use SWITCH-GVRP-MAXIMUM-VLANS to specify an appropriate size.\n",
        gvrp->vlansMax, vlanId);
    ERROR_ReportError(errString);
}


// NAME:        SwitchGvrp_ReceiveMsg
//
// PURPOSE:     Process one received message.
//
// PARAMETERS:  Node data.
//              SwitchGvrp data.
//              Port GarpGid.
//              Received message.
//
// RETURN:      None.

static
void SwitchGvrp_ReceiveMsg(
    Node* node,
    SwitchGvrp* gvrp,
    GarpGid* portGid,
    GvrpGvfMsg* gvfMsg)
{
    unsigned gidIndex = GVRP_UNUSED_GID_INDEX;

    if ((gvfMsg->event == GARP_GID_RX_LEAVE_ALL)
        || (gvfMsg->event == GARP_GID_RX_LEAVE_ALL_RANGE))
    {
        GarpGid_ReceiveLeaveAll(portGid);
    }
    else
    {
        if (!GvrpGvd_FindEntry(gvrp, gvfMsg->key1, &gidIndex))
        {
            // If no entry is found, Leave and Empty messages
            // can be discarded.
            //
            if ((gvfMsg->event == GARP_GID_RX_LEAVE_EMPTY)
                || (gvfMsg->event == GARP_GID_RX_LEAVE_IN))
            {
                return;
            }

            if ((gvfMsg->event == GARP_GID_RX_JOIN_IN)
                || (gvfMsg->event == GARP_GID_RX_JOIN_EMPTY))
            {
                if (!GvrpGvd_CreateEntry(gvrp, gvfMsg->key1, &gidIndex))
                {
                    if (GarpGid_FindUnused(&gvrp->garp, 0, &gidIndex))
                    {
                        GvrpGvd_DeleteEntry(gvrp, gidIndex);
                        (void) GvrpGvd_CreateEntry(
                            gvrp, gvfMsg->key1, &gidIndex);
                    }
                    else
                    {
                        char traceStr[MAX_STRING_LENGTH];

                        // Database full.
                        //
                        SwitchGvrp_DbFull(gvrp, portGid, gvfMsg->key1);
                        gidIndex = GVRP_UNUSED_GID_INDEX;

                        sprintf(traceStr,
                            "Db full. Join msg for %u discarded.",
                            gvfMsg->key1);
                        SwitchGvrp_Trace(node, portGid, NULL, traceStr);
                    }
                }
            }
        }

        if (gidIndex != GVRP_UNUSED_GID_INDEX)
        {
            GarpGid_ReceiveMsg(node, portGid, gidIndex, gvfMsg->event);
        }
    }
}


// NAME:        SwitchGvrp_BypassThisAttribType
//
// PURPOSE:     Bypass the attribute list for an unknown attribute type.
//
// PARAMETERS:  Pointer to attribute list.
//
// RETURN:      Pointer to next list.

static char* SwitchGvrp_BypassThisAttribType(
    char* garpAttributes)
{
    unsigned char length = *garpAttributes;
    garpAttributes++;

    while (length == GARP_END_OF_ATTRIBUTE)
    {
        garpAttributes += length;
        length = *garpAttributes;
        garpAttributes++;
    }

    return garpAttributes;
}


// NAME:        SwitchGvrp_Receive
//
// PURPOSE:     Process an entire received pdu for this instance of GVRP.
//
// PARAMETERS:  Node data.
//              SwitchGvrp data.
//              Port GarpGid.
//              Received PDU.
//
// RETURN:      None.

static
void SwitchGvrp_Receive(
    Node* node,
    void* garpPtr,
    void* portGidPtr,
    Message* msg)
{
    GarpGid* portGid = (GarpGid*) portGidPtr;
    MacSwitch* sw = (MacSwitch*) Switch_GetSwitchData(node);
    SwitchGvrp* gvrp = (SwitchGvrp*) SwitchGvrp_GetGvrpData(sw);

    GvrpGvfMsg gvfMsg;
    unsigned short protocolId = 0;
    char* garpAttributes = (char*) MESSAGE_ReturnPacket(msg);

    // Collect protocol ID.
    //
    protocolId = (unsigned short) ((*garpAttributes) * GVRP_BASE_VALUE);
    garpAttributes++;
    protocolId = (unsigned short) (protocolId + (*garpAttributes));
    garpAttributes++;

    // Port received GVRP PDU with invaild
    // protocol ID. PDU discarded.
    //
    if (protocolId != GARP_PROTOCOL_ID)
    {
        char trcString[MAX_STRING_LENGTH];
        sprintf(trcString,
            "Msg received with unknown protocol Id %d. Msg discarded.\n",
            node->nodeId);
        SwitchGvrp_Trace(node, portGid, msg, trcString);

        MESSAGE_Free(node, msg);
        return;
    }

    SwitchGvrp_Trace(node, portGid, msg, "Receive");

    // Received a valid PDU. Read first attribute.
    //
    gvfMsg.attribute = (GvrpAttributeType)(*garpAttributes);
    garpAttributes++;

    while (gvfMsg.attribute != GARP_END_OF_PDU)
    {
        // Bypass the attribute list for an unrecognized
        // Attribute type present in PDU.
        //
        while (gvfMsg.attribute != GVRP_VLAN_ATTRIBUTE)
        {
            if (gvfMsg.attribute == GARP_END_OF_PDU)
            {
                MESSAGE_Free(node, msg);
                return;
            }

            garpAttributes = SwitchGvrp_BypassThisAttribType(garpAttributes);

            gvfMsg.attribute = (GvrpAttributeType)(*garpAttributes);
            garpAttributes++;
        }

        gvfMsg.length = *garpAttributes;
        garpAttributes++;

        while (gvfMsg.length != 0)
        {
            // Get and decode event.
            //
            Garp_DecodeReceivedEvent((GarpGidEvent)(*garpAttributes),
                                     &gvfMsg.event);
            garpAttributes++;

            SwitchGvrp_UpdatePortStats(
                sw, gvrp, portGid->portNumber, gvfMsg.event);

            switch (gvfMsg.event)
            {
                case GARP_GID_RX_LEAVE_ALL:

                    ERROR_Assert(gvfMsg.length == 2,
                        "SwitchGvrp_Receive: "
                        "Invalid attribute length for LeaveAll event.\n");
                    SwitchGvrp_ReceiveMsg(node, gvrp, portGid, &gvfMsg);
                    break;

                case GARP_GID_RX_JOIN_EMPTY:
                case GARP_GID_RX_JOIN_IN:
                case GARP_GID_RX_LEAVE_EMPTY:
                case GARP_GID_RX_LEAVE_IN:

                    // Key is two bytes long
                    //
                    ERROR_Assert(gvfMsg.length == 4,
                        "SwitchGvrp_Receive: "
                        "Invalid attribute length for attribute event.\n");

                    gvfMsg.key1 = (unsigned short) (*garpAttributes * GVRP_BASE_VALUE);
                    garpAttributes++;
                    gvfMsg.key1 = (unsigned short) (gvfMsg.key1 + (*garpAttributes));
                    garpAttributes++;
                    SwitchGvrp_ReceiveMsg(node, gvrp, portGid, &gvfMsg);
                    break;

                default:
                    garpAttributes += gvfMsg.length;
                    break;
            }

            // Get next attribute from attribute list.
            //
            gvfMsg.length = *garpAttributes;
            garpAttributes++;
        } //do while not end of attribute list

        // All attributes at previous list has decoded.
        // Get next list for other attribute type.
        //
        gvfMsg.attribute = (GvrpAttributeType)(*garpAttributes);
        garpAttributes++;
    } //do while not end of pdu

    MESSAGE_Free(node, msg);
}


// -------------------------------------------------------------------------
// Transmit processing

// NAME:        SwitchGvrp_Transmit
//
// PURPOSE:     Get and prepare a pdu for transmission.
//              If there is no message to send, simply return.
//              If there is more to transmit than can be held
//              in a single pdu, GID will reschedule a call
//              to this function.
//
// PARAMETERS:  Node data.
//              SwitchGvrp data.
//              Port GarpGid.
//
// RETURN:      None.

static
void SwitchGvrp_Transmit(
    Node* node,
    void* garpPtr,
    void* portGidPtr)
{
    GarpGid* portGid = (GarpGid*) portGidPtr;
    MacSwitch* sw = (MacSwitch*) Switch_GetSwitchData(node);
    SwitchGvrp* gvrp = (SwitchGvrp*) SwitchGvrp_GetGvrpData(sw);

    Message* garpPdu;
    GvrpGvfMsg gvfMsg;
    GarpGidEvent transmitEvent;
    BOOL hasAttributeToSend = FALSE;
    unsigned gidIndex;
    int garpPduLength = 0;
    char garpAttributes[GARP_PDU_LENGTH_MAX];
    BOOL isEnqueued = FALSE;

    // Encode protocol ID field
    //
    garpAttributes[0] = GARP_PROTOCOL_ID / GVRP_BASE_VALUE;
    garpAttributes[1] = GARP_PROTOCOL_ID % GVRP_BASE_VALUE;
    garpPduLength = 2;

    // Encode attribute type value.
    // GVRP defines a single attribute type with value 1.
    //
    garpAttributes[garpPduLength++] = GVRP_VLAN_ATTRIBUTE;

    transmitEvent = GarpGid_NextTx(node, portGid, &gidIndex);
    if (transmitEvent != GARP_GID_NULL)
    {
        do
        {
            gvfMsg.event = transmitEvent;
            if (gvfMsg.event == GARP_GID_TX_LEAVE_ALL)
            {
                gvfMsg.length = 2;
            }
            else
            {
                if (!GvrpGvd_GetKey(gvrp, gidIndex, &gvfMsg.key1))
                {
                    continue;
                }
                gvfMsg.length = 4;
            }

            // At least one attribute found to be sent.
            //
            hasAttributeToSend = TRUE;

            // Check if sufficient space is available to carry
            // this attribute value.
            //
            if (garpPduLength + gvfMsg.length >= GARP_PDU_LENGTH_MAX - 2)
            {
                GarpGid_Untransmit(portGid);
                break;
            }

            // Pdu has enough space to carry this attribute.
            // So, increase statistics.
            //
            SwitchGvrp_UpdatePortStats(sw, gvrp, portGid->portNumber,
                                       gvfMsg.event);

            // Write this new message into PDU.
            //
            garpAttributes[garpPduLength] = gvfMsg.length;
            garpAttributes[garpPduLength + 1] = (char) gvfMsg.event;
            if (gvfMsg.length == 4)
            {
                garpAttributes[garpPduLength + 2] = (char) (gvfMsg.key1
                                                    / GVRP_BASE_VALUE);
                garpAttributes[garpPduLength + 3] = (char) (gvfMsg.key1
                                                    % GVRP_BASE_VALUE);
            }
            garpPduLength += gvfMsg.length;

            if (gvfMsg.event == GARP_GID_TX_LEAVE_ALL)
            {
                break;
            }

        } while((transmitEvent = GarpGid_NextTx(node, portGid, &gidIndex))
                != GARP_GID_NULL);

        // Check if atleast one attribute has found to send.
        // Append end of attribute mark and also add end of
        // PDU mark as GVRP advertises only one attribute type.
        //
        if (!hasAttributeToSend)
        {
            return;
        }
        garpAttributes[garpPduLength++] = GARP_END_OF_ATTRIBUTE;
        garpAttributes[garpPduLength++] = GARP_END_OF_PDU;

        // Attribute list collection is over.
        // Construct the PDU to send and pass it to send frame entity.
        //
        garpPdu = MESSAGE_Alloc(node, 0, 0, 0);
        MESSAGE_PacketAlloc(node, garpPdu, garpPduLength, TRACE_GVRP);
        memcpy((char*)(MESSAGE_ReturnPacket(garpPdu)),
            garpAttributes, garpPduLength);

        SwitchGvrp_Trace(node, portGid, garpPdu, "Send");

        Mac802Address macAddr;
        MAC_CopyMac802Address(&macAddr, &gvrp->GVRP_Group_Address);

        Switch_SendFrameToMac(
            node, garpPdu, CPU_INTERFACE,
            portGid->interfaceIndex,
            macAddr, sw->swAddr, SWITCH_VLAN_ID_GVRP,
            GVRP_FRAME_PRIORITY, &isEnqueued);

        if (!isEnqueued)
        {
            MESSAGE_Free(node, garpPdu);
            SwitchGvrp_Trace(node, portGid, NULL,
                "Unable to send GVRP PDU; Q is full.");
        }
    }
}


// -------------------------------------------------------------------------

// NAME:        SwitchGvrp_CreateStat
//
// PURPOSE:     Allocate space to hold statistics for gvrp
//
// PARAMETERS:  Node data.
//              switch data.
//
// RETURN:      TRUE, if created.
//              FALSE, otherwise.

static
BOOL SwitchGvrp_CreateStat(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput,
    SwitchGvrp* gvrp)
{
    GvrpStat* tempStat = NULL;
    SwitchPort* aPort = NULL;
    BOOL retVal = FALSE;
    char buf[MAX_STRING_LENGTH];


    // Initialize statistics related variables.
    // Format is [node list] SWITCH-GVRP-STATISTICS NO | YES
    // Default is NO
    //
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-GVRP-STATISTICS",
        &retVal,
        buf);

    gvrp->printStats = GVRP_PRINT_STATS_DEFAULT;
    if (retVal)
    {
        if (!strcmp(buf, "YES"))
        {
            gvrp->printStats = TRUE;
        }
        else if (!strcmp(buf, "NO"))
        {
            gvrp->printStats = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "SwitchGvrp_CreateStat: "
                "Error in SWITCH-GVRP-STATISTICS specification.\n"
                "Expecting YES or NO\n");
        }
    }

    gvrp->stats = (GvrpStat*)
        MEM_malloc(sizeof(GvrpStat) * gvrp->numPorts);
    memset(gvrp->stats, 0, sizeof(GvrpStat) * gvrp->numPorts);

    tempStat = gvrp->stats;
    aPort = sw->firstPort;
    while (aPort)
    {

        MAC_CopyMac802Address(&tempStat->portAddr, &aPort->portAddr);
        tempStat++;
        aPort = aPort->nextPort;
    }

    return TRUE;
}


// NAME:        SwitchGvrp_CreateGvrp
//
// PURPOSE:     Create SwitchGvrp data in a switch.
//
// PARAMETERS:  Node data.
//              switch data.
//              NodeInput.
//              Max vlan to handle.
//
// RETURN:      TRUE, if created.
//              FALSE, otherwise.

static
BOOL SwitchGvrp_CreateGvrp(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput,
    unsigned vlansMax)
{
    SwitchGvrp* gvrp = NULL;

    ERROR_Assert(sw->gvrp == NULL,
        "SwitchGvrp_CreateGvrp: GVRP already initialized.\n");

    sw->gvrp = (SwitchGvrp*) MEM_malloc(sizeof(SwitchGvrp));
    memset(sw->gvrp, 0, sizeof(SwitchGvrp));
    gvrp = sw->gvrp;

    // Initialize the Garp structure.
    //
    Garp_Init(node, &gvrp->garp);

    // Create a new instance of GIP for this application.
    //
    GarpGip_CreateGip(vlansMax, &gvrp->garp.gip);

    // Create internal VLAN database.
    //
    GvrpGvd_CreateGvd(vlansMax, &gvrp->gvd);

    //.Initialize different indices.
    //
    gvrp->vlansMax = vlansMax;
    gvrp->garp.maxGidIndex = vlansMax - 1;
    gvrp->garp.lastGidIndex = 0;
    gvrp->numGvdEntries = vlansMax;


    // Assign application specific functions.
    //
    gvrp->garp.joinIndicationFn = SwitchGvrp_JoinIndication;
    gvrp->garp.leaveIndicationFn = SwitchGvrp_LeaveIndication;

    gvrp->garp.joinPropagatedFn = SwitchGvrp_JoinLeavePropagated;
    gvrp->garp.leavePropagatedFn = SwitchGvrp_JoinLeavePropagated;

    gvrp->garp.transmitFn = SwitchGvrp_Transmit;
    gvrp->garp.receiveFn = SwitchGvrp_Receive;

    gvrp->garp.addNewPortFn = SwitchGvrp_AddNewPort;
    gvrp->garp.removePortFn = SwitchGvrp_RemovePort;

    return TRUE;
}


// NAME:        SwitchGvrp_Init
//
// PURPOSE:     Initialize GVRP and related variables.
//
// PARAMETERS:  Node data
//              Switch data
//              NodeInput
//
// RETURN:      NONE
//

void SwitchGvrp_Init(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    int intInput;
    int vlansMax = 0;
    clocktype timeInput;
    BOOL retVal = FALSE;
    char buf[MAX_STRING_LENGTH];
    char errString[MAX_STRING_LENGTH];
    SwitchPort* port = sw->firstPort;
    SwitchGvrp* gvrp = NULL;

    // Check if SWITCH-RUN-GVRP is enabled for this switch
    // Format is [node list] SWITCH-RUN-GVRP NO | YES
    // Default is NO
    //
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-RUN-GVRP",
        &retVal,
        buf);

    if (retVal)
    {
        if (!strcmp(buf, "NO"))
        {
            sw->runGvrp = FALSE;
        }
        else if (!strcmp(buf, "YES"))
        {
            sw->runGvrp = TRUE;
        }
        else
        {
            sprintf(errString,
                "SwitchGvrp_Init: "
                "Error in configuration of SWITCH-RUN-GVRP for node %u.\n"
                "Options are YES or NO.\n",
                node->nodeId);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->runGvrp = FALSE;
    }

    // Return if GVRP is not to be executed.
    //
    if (sw->runGvrp == FALSE)
    {
        return;
    }


    // Get an estimate of number of VLANs to size for
    // Format is [node list] SWITCH-GVRP-MAXIMUM-VLANS <value>
    // Default is SWITCH_GVRP_VLANS_MAX (10)
    //
    IO_ReadInt(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-GVRP-MAXIMUM-VLANS",
        &retVal,
        &intInput);

    if (retVal)
    {
        if (intInput > 0 && intInput <= SWITCH_VLAN_ID_MAX)
        {
            vlansMax = intInput;
        }
        else
        {
            sprintf(errString,
                "SwitchGvrp_Init: "
                "Error in SWITCH-GVRP-MAXIMUM-VLANS in user configuration "
                "for node %u.\n"
                "Number of VLANs should be between %d to %d.\n"
                "Default value is %d.\n",
                node->nodeId,
                SWITCH_VLAN_ID_MIN, SWITCH_VLAN_ID_MAX,
                SWITCH_GVRP_VLANS_MAX);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        vlansMax = SWITCH_GVRP_VLANS_MAX;
    }


    // Create GVRP application instance.
    //
    SwitchGvrp_CreateGvrp(node, sw, nodeInput, vlansMax);
    gvrp = sw->gvrp;


    // Time values are assumed to be same across the
    // switched network.

    // Read join time
    // Format is SWITCH-GARP-JOIN-TIME <value>
    // Default is as per Table 12.10
    //
    IO_ReadTime(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-GARP-JOIN-TIME",
        &retVal,
        &timeInput);

    if (retVal)
    {
        if (timeInput > 0)
        {
            sw->joinTime = timeInput;
        }
        else
        {
            sprintf(errString,
                "SwitchGvrp_Init: "
                "Error in SWITCH-GARP-JOIN-TIME in user configuration.\n"
                "Join timer should have a positive value.\n");
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->joinTime = GARP_GID_JOIN_TIME_DEFAULT;
    }

    // Read leave time
    // Format is SWITCH-GARP-LEAVE-TIME <value>
    // Default is as per Table 12.10
    //
    IO_ReadTime(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-GARP-LEAVE-TIME",
        &retVal,
        &timeInput);

    if (retVal)
    {
        if (timeInput > 0)
        {
            sw->leaveTime = timeInput;
        }
        else
        {
            sprintf(errString,
                "SwitchGvrp_Init: "
                "Error in SWITCH-GARP-LEAVE-TIME in user configuration.\n"
                "Leave timer should have a positive value.\n");
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->leaveTime = GARP_GID_LEAVE_TIME_DEFAULT;
    }

    // Read leave all time
    // Format is SWITCH-GARP-LEAVEALL-TIME <value>
    // Default is as per Table 12.10
    //
    IO_ReadTime(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-GARP-LEAVEALL-TIME",
        &retVal,
        &timeInput);

    if (retVal)
    {
        if (timeInput > 0)
        {
            sw->leaveallTime = timeInput;
        }
        else
        {
            sprintf(errString,
                "SwitchGvrp_Init: "
                "Error in SWITCH-GARP-LEAVEALL-TIME in user configuration.\n"
                "Leaveall timer should have a positive value.\n");
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->leaveallTime = GARP_GID_LEAVEALL_TIME_DEFAULT;
    }

    // Validate time values
    // Note: The standard specifies that leave time should
    // be at least twice join time. Since the join time is
    // randomized over 0.5 to 1 times its value plus possible
    // hold time, the validity check used thrice join time.
    if (sw->leaveTime < 3 * sw->joinTime)
    {
        char joinTimeStr[MAX_STRING_LENGTH];
        char leaveTimeStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(sw->joinTime, joinTimeStr);
        TIME_PrintClockInSecond(sw->leaveTime, leaveTimeStr);
        sprintf(errString,
            "SwitchGvrp_Init: "
            "Leave Time %s should be equal or more than "
            "thrice Join Time %s.\n",
            leaveTimeStr, joinTimeStr);
        ERROR_ReportError(errString);
    }
    if (sw->leaveallTime <= sw->leaveTime)
    {
        char leaveallTimeStr[MAX_STRING_LENGTH];
        char leaveTimeStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(sw->leaveallTime, leaveallTimeStr);
        TIME_PrintClockInSecond(sw->leaveTime, leaveTimeStr);
        sprintf(errString,
            "SwitchGvrp_Init: "
            "Leave All Time %s should be larger than Leave Time %s.\n",
            leaveallTimeStr, leaveTimeStr);
        ERROR_ReportError(errString);
    }
    if (sw->leaveallTime < 10 * sw->leaveTime)
    {
        char leaveallTimeStr[MAX_STRING_LENGTH];
        char leaveTimeStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(sw->leaveallTime, leaveallTimeStr);
        TIME_PrintClockInSecond(sw->leaveTime, leaveTimeStr);
        sprintf(errString,
            "SwitchGvrp_Init: "
            "Leave All Time %s should be much larger "
            "(say, 10 times) than Leave Time %s.\n",
            leaveallTimeStr, leaveTimeStr);
        ERROR_ReportWarning(errString);
    }


    // Create and initialize GARP instances as required
    // by GVRP, for each port at this switch.
    // Allocate space to collect GVRP statistics.
    //
    gvrp->numPorts = 0;
    memcpy(&(gvrp->GVRP_Group_Address).byte, gvrpGroupAddress,
                                                MAC_ADDRESS_LENGTH_IN_BYTE);
    port = sw->firstPort;
    while (port)
    {
        GarpGid_CreatePort(
            node, &gvrp->garp, port->macData->interfaceIndex,
            port->portNumber);

        // Count number of ports configured at this switch.
        //
        gvrp->numPorts++;

        port = port->nextPort;
    }

    SwitchGvrp_CreateStat(node, sw, nodeInput, gvrp);

    SwitchGvrp_TraceInit(node, sw, nodeInput);
}


// NAME:        SwitchGvrp_DestroyStat
//
// PURPOSE:     Release allocated space for statistics.
//
// PARAMETERS:  Pointer to stat.
//
// RETURN:      None.

static
void SwitchGvrp_DestroyStat(GvrpStat* stats)
{
    MEM_free(stats);
}


// NAME:        SwitchGvrp_DestroyGvrp
//
// PURPOSE:     Release the allocated space for gvrp in a switch.
//
// PARAMETERS:  Node data.
//              SwitchGvrp data.
//
// RETURN:      TRUE, if created.
//              FALSE, otherwise.

static
void SwitchGvrp_DestroyGvrp(
    Node* node,
    SwitchGvrp* gvrp)
{
    GarpGid* portGid = NULL;

    while ((portGid = gvrp->garp.gid) != NULL)
    {
        GarpGid_DestroyPort(node, &gvrp->garp, portGid->portNumber);
    }
    GvrpGvd_DestroyGvd(gvrp->gvd);
    GarpGip_DestroyGip(gvrp->garp.gip);
    SwitchGvrp_DestroyStat(gvrp->stats);
    MEM_free(gvrp);
}


// NAME:        SwitchGvrp_PrintStats
//
// PURPOSE:     Print Gvrp statistics.
//
// PARAMETERS:  Node data
//              SwitchGvrp data
//
// RETURN:      None

static
void SwitchGvrp_PrintStats(
    Node* node,
    SwitchGvrp* gvrp)
{
    unsigned short aCount = 0;
    GvrpStat* portStats = NULL;
    char buf[MAX_STRING_LENGTH];
    char portAddrStr[MAX_STRING_LENGTH];

    for (aCount = 0; aCount < gvrp->numPorts; aCount++)
    {
        portStats = (GvrpStat*) (gvrp->stats + aCount);

        if (portStats == NULL)
            continue;

       NodeAddress ipAddress = DefaultMac802AddressToIpv4Address(node,
                                                   &portStats->portAddr);
       IO_ConvertIpAddressToString(ipAddress, portAddrStr);

        sprintf(buf, "Join Empty received = %u",
            portStats->joinEmptyReceived);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);

        sprintf(buf, "Join In received = %u",
            portStats->joinInReceived);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);

        sprintf(buf, "Leave All received = %u",
            portStats->leaveAllReceived);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);

        sprintf(buf, "Leave Empty received = %u",
            portStats->leaveEmptyReceived);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);

        sprintf(buf, "Leave In received = %u",
            portStats->leaveInReceived);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);

        sprintf(buf, "Join Empty transmitted = %u",
            portStats->joinEmptyTransmitted);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);

        sprintf(buf, "Join In transmitted = %u",
            portStats->joinInTransmitted);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);

        sprintf(buf, "Leave All transmitted = %u",
            portStats->leaveAllTransmitted);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);

        sprintf(buf, "Leave Empty transmitted = %u",
            portStats->leaveEmptyTransmitted);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);

        sprintf(buf, "Leave In transmitted = %u",
            portStats->leaveInTransmitted);
        IO_PrintStat(node, "Mac-Switch", "Gvrp",
            portAddrStr, -1, buf);
    }
}


// NAME:        SwitchGvrp_Finalize
//
// PURPOSE:     Call function to print Gvrp statistics and
//              release statistic structure.
//
// PARAMETERS:  Node data
//              Switch data
//
// RETURN:      NONE
//

void SwitchGvrp_Finalize(
    Node* node,
    MacSwitch* sw)
{
    SwitchGvrp* gvrp = (SwitchGvrp*) SwitchGvrp_GetGvrpData(sw);

    // GVRP not enabled at this switch.
    //
    if (!sw->runGvrp)
    {
        return;
    }

    // Print GVRP statistics.
    //
    if (gvrp->printStats)
    {
        SwitchGvrp_PrintStats(node, gvrp);
    }

    // Release allocated space for GVRP application.
    //
    SwitchGvrp_DestroyGvrp(node, gvrp);
}

// -------------------------------------------------------------------------
