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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"

#include "mac.h"
#include "mac_switch.h"

const unsigned char switchNumberMask[3]= {0xff,0xff,0xff};

// ------------------------------------------------------------------
// STP library functions


// NAME:        SwitchStp_ClocktypeToBpduTime
//
// PURPOSE:     Convert clocktype to BPDU time.
//              BPDU time is encoded as 1/256 S
//
// PARAMETERS:  time as clocktype
//
// RETURN:      BPDU time value

static
StpBpduTime SwitchStp_ClocktypeToBpduTime(
    clocktype timeVal)
{
    return (StpBpduTime)(timeVal / SECOND * STP_BPDU_TIME_UNIT);
}


// NAME:        SwitchStp_BpduTimeToClocktype
//
// PURPOSE:     Convert BPDU time to clocktype
//              BPDU time is encoded as 1/256 S
//
// PARAMETERS:  time as BPDU time
//
// RETURN:      clocktype time value

static
clocktype SwitchStp_BpduTimeToClocktype(
    StpBpduTime timeVal)
{
    return timeVal * SECOND / STP_BPDU_TIME_UNIT;
}


// NAME:        SwitchStp_DecrementTimerBy1Second
//
// PURPOSE:     To decrement the state machine timers by a second.
//              This is done for every tick event.
//
//              The timers become zero if they are smaller than
//              one second.
//
// PARAMETERS:  timer value
//
// RETURN:      decremented timer value

static
void SwitchStp_DecrementTimerBy1Second(
    clocktype* const timeVal)
{
    if (*timeVal > SECOND)
    {
        *timeVal -= SECOND;
    }
    else
    {
        *timeVal = 0;
    }
}


// NAME:        SwitchStp_ClocktypeToNearestSecond
//
// PURPOSE:     Convert clocktype to nearest second.
//              This is used for updates on receipt of a BPDU.
//
// PARAMETERS:  the time to round off
//
// RETURN:      time rounded to nearest second.

static
clocktype SwitchStp_ClocktypeToNearestSecond(
    clocktype theTime)
{
    return ((int)(SECOND / 2 + theTime) / SECOND) * SECOND;
}


//...................................................................
// Functions related to assignment of Priority, Vector, Timers


// NAME:        SwitchStp_AssignSwId
//
// PURPOSE:     Assign switch ID structure with given values of
//              priority and address
//
// PARAMETERS:  switch priority
//              switch address
//
// RETURN:      assigned switch ID structure

static
void SwitchStp_AssignSwId(
    unsigned short priority,
    Mac802Address ipAddr,
    StpSwitchId* const swId)
{
    swId->priority = priority;
    MAC_CopyMac802Address(&swId->ipAddr, &ipAddr);
}


// NAME:        SwitchStp_AssignPortId
//
// PURPOSE:     Assign port ID structure with given values of
//              priority and port number
//
// PARAMETERS:  port priority
//              port number
//
// RETURN:      assigned port ID structure
//
// NOTES:       The current port ID structure uses unsigned char
//              values for priority and port number. According
//              to the standard they should be 4 and 12 bits
//              respectively. In case the port ID structure is
//              changed, the unsigned char cast needs to be
//              removed; the assert is a reminder.

static
void SwitchStp_AssignPortId(
    unsigned char priority,
    unsigned short portNumber,
    StpPortId* const portId)
{
    ERROR_Assert(portNumber <= SWITCH_NUMBER_OF_PORTS_MAX,
        "SwitchStp_AssignPortId: port number larger than permitted.\n");

    portId->priority = priority;

    ERROR_Assert(portNumber <= 255,
        "SwitchStp_AssignPortId: Need to change cast "
        "from unsigned char if port number is made larger 255.\n");

    portId->portNumber = (unsigned char) portNumber;
}


// NAME:        SwitchStp_AssignPriority
//
// PURPOSE:     Assign the priority structure with given values.
//
// PARAMETERS:  root switch ID structure
//              root path cost
//              switch ID structure
//              port ID structure
//
// RETURN:      Assigned priority structure

static
void SwitchStp_AssignPriority(
    StpSwitchId rootSwId,
    UInt32 rootCost,
    StpSwitchId swId,
    StpPortId portId,
    StpPriority* const stpPriority)
{
    stpPriority->rootSwId = rootSwId;
    stpPriority->rootCost = rootCost;
    stpPriority->swId = swId;
    stpPriority->txPortId = portId;
}


// NAME:        SwitchStp_AssignTimes
//
// PURPOSE:     Assign the STP time structure with given values.
//
// PARAMETERS:  message age
//              max age
//              hello time
//              forward delay
//
// RETURN:      Assigned STP time structure

static
void SwitchStp_AssignTimes(
    clocktype messageAge,
    clocktype maxAge,
    clocktype helloTime,
    clocktype forwardDelay,
    StpTimes* const stpTimes)
{
    stpTimes->messageAge = messageAge;
    stpTimes->maxAge = maxAge;
    stpTimes->helloTime = helloTime;
    stpTimes->forwardDelay = forwardDelay;
}


//...................................................................
// Functions related to comparison of IDs, Priority, Vector, Times



// All these functions compare the two given parameters.
//
// The comparison may be for two switch ID structures, each
// containing switch priority and switch address. A lower
// value is considered better with switch priority being
// the more significant value.
//
// For port ID structures, each containing port priority and
// port numbers, a lower value is considered better with port
// priority being the more significant value.

// For path costs, it is a simple numerical comparison but is
// written to provide consistent return values with other
// related comparisons of switch IDs, port IDs, priority
// structures and vectors.
//
// For priority structures and vectors, the comparison is
// ordered as root ID, path cost, switch ID, tx port ID,
// rx port ID in decreasing significane.
//
// Return value of these comparisons is:
//      STP_FIRST_IS_BETTER if first is better, or
//      STP_BOTH_ARE_SAME, or
//      STP_FIRST_IS_WORSE
//
// When comparing times, there is no ordered sequence and
// the return value is one of
//      STP_BOTH_ARE_SAME, or
//      STP_BOTH_ARE_DIFFERENT



// NAME:        SwitchStp_CompareSwitchId
//
// PURPOSE:     Compares the two given switch ID structures
//              containing switch priority and switch address.
//              A lower value is considered better with switch
//              priority being the more important value.
//
// PARAMETERS:  first switch ID structure
//              second switch ID structure
//
// RETURN:      either
//                  STP_FIRST_IS_BETTER if first is better,
//                  STP_BOTH_ARE_SAME, or
//                  STP_FIRST_IS_WORSE

static
StpComparisonType SwitchStp_CompareSwitchId(
    const StpSwitchId* const first,
    const StpSwitchId* const second)
{
    if (first->priority < second->priority)
    {
        return STP_FIRST_IS_BETTER;
    }
    else if (first->priority == second->priority)
    {

        if (memcmp(&first->ipAddr.byte,&second->ipAddr.byte,
                    MAC_ADDRESS_DEFAULT_LENGTH) < 0)
        {
            return STP_FIRST_IS_BETTER;
        }
        else if (memcmp(&first->ipAddr.byte,&second->ipAddr.byte,
                    MAC_ADDRESS_DEFAULT_LENGTH) == 0)
        {
            return STP_BOTH_ARE_SAME;
        }
    }
    return STP_FIRST_IS_WORSE;
}


// NAME:        SwitchStp_ComparePortId
//
// PURPOSE:     Compares the two given port ID structures
//              containing port priority and port number.
//              A lower value is considered better with port
//              priority being the more important value.
//
// PARAMETERS:  first port ID structure
//              second port ID structure
//
// RETURN:      either
//                  STP_FIRST_IS_BETTER if first is better,
//                  STP_BOTH_ARE_SAME, or
//                  STP_FIRST_IS_WORSE

static
StpComparisonType SwitchStp_ComparePortId(
    const StpPortId* const first,
    const StpPortId* const second)
{
    if (first->priority < second->priority)
    {
        return STP_FIRST_IS_BETTER;
    }
    else if (first->priority == second->priority)
    {
        if (first->portNumber < second->portNumber)
        {
            return STP_FIRST_IS_BETTER;
        }
        else if (first->portNumber == second->portNumber)
        {
            return STP_BOTH_ARE_SAME;
        }
    }
    return STP_FIRST_IS_WORSE;
}


// NAME:        SwitchStp_ComparePathCost
//
// PURPOSE:     Compares the two given root path costs.
//              This is a simple numerical comparison but is
//              written to provide consistent return values
//              with similar comparisons of switch IDs, port IDs,
//              priority structures and vectors.
//
// PARAMETERS:  first path cost
//              second path cost
//
// RETURN:      either
//                  STP_FIRST_IS_BETTER if first is better,
//                  STP_BOTH_ARE_SAME, or
//                  STP_FIRST_IS_WORSE

static
StpComparisonType SwitchStp_ComparePathCost(
    int first,
    int second)
{
    if (first < second)
    {
        return STP_FIRST_IS_BETTER;
    }
    else if (first == second)
    {
        return STP_BOTH_ARE_SAME;
    }
    return STP_FIRST_IS_WORSE;
}


static
StpComparisonType SwitchStp_ComparePriority(
    const StpPriority* const first,
    const StpPriority* const second)
{
    StpComparisonType result;

    result = SwitchStp_CompareSwitchId(&first->rootSwId, &second->rootSwId);
    if (result == STP_BOTH_ARE_SAME)
    {
        result = SwitchStp_ComparePathCost(
            first->rootCost, second->rootCost);
        if (result == STP_BOTH_ARE_SAME)
        {
            result = SwitchStp_CompareSwitchId(
                &first->swId, &second->swId);
            if (result == STP_BOTH_ARE_SAME)
            {
                result = SwitchStp_ComparePortId(
                    &first->txPortId, &second->txPortId);
            }
        }
    }
    return result;
}


static
StpComparisonType SwitchStp_CompareVector(
    const StpVector* const first,
    const StpVector* const second)
{
    StpComparisonType result;

    result = SwitchStp_ComparePriority(
        &first->stpPriority, &second->stpPriority);
    if (result == STP_BOTH_ARE_SAME)
    {
        result = SwitchStp_ComparePortId(
            &first->rxPortId, &second->rxPortId);
    }
    return result;
}


static
StpComparisonType SwitchStp_CompareTimes(
    const StpTimes* const first,
    const StpTimes* const second)
{
    if (first->maxAge == second->maxAge
        && first->helloTime == second->helloTime
        && first->forwardDelay == second->forwardDelay
        && first->messageAge == second->messageAge)
    {
        return STP_BOTH_ARE_SAME;
    }
    return STP_BOTH_ARE_DIFFERENT;
}


//...................................................................
// Functions related to BPDUs


// NAME:        SwitchStp_GetBpduType
//
// PURPOSE:     Determine the BPDU type from the message.
//              This type is encoded in the 4th byte of the
//              message payload and has a dependancy on the
//              the protocol version encoded in the 3rd octet.
//
// PARAMETERS:  BPDU message
//
// RETURN:      Either CONFIG, TCN, RST or INVALID

static
StpBpduType SwitchStp_GetBpduType(
    const Message* const bpdu)
{
    StpBpduHdr* hdr = (StpBpduHdr*) MESSAGE_ReturnPacket(bpdu);

    if (hdr->protocolVersion == STP_PROTOCOL_VERSION)
    {
        if (hdr->bpduType == STP_BPDU_TYPE)
        {
            return STP_BPDUTYPE_CONFIG;
        }
        else if (hdr->bpduType == STP_TCN_TYPE)
        {
            return STP_BPDUTYPE_TCN;
        }
    }
    else if (hdr->protocolVersion == STP_RST_PROTOCOL_VERSION)
    {
        if (hdr->bpduType == STP_RST_TYPE)
        {
            return STP_BPDUTYPE_RST;
        }
    }

    return STP_BPDUTYPE_INVALID;
}


// NAME:        SwitchStp_PrepareBpdu
//
// PURPOSE:     Write common fields of a Config or RST BPDU.
//              For the BPDU header, this is the protocol ID.
//              For the body, this is the priority and time
//              fields.
//
//              The calling function is expected to fill in
//              the BPDU version and type, and the flag bit
//              fields.
//
// PARAMETERS:  BPDU pointer
//
// RETURN:      Assigned bpdu

static
void SwitchStp_PrepareBpdu(
    const Node* const node,
    const MacSwitch* const sw,
    const SwitchPort* const port,
    StpConfigBpdu* const bpdu)
{
    bpdu->bpduHdr.protocolId = STP_BPDU_ID;

    // The values sent as switch ID and port ID are
    // those that that reflect what this ports currently
    // believes are those of the designated switch and
    // designated port of the connected LAN.
    // This is in contrast to what is stated in 802.1q,
    // section 17.7, para 4, viz the transmitting switch
    // ID and transmitting port ID.
    //
    bpdu->rootId = port->portPriority.rootSwId;
    bpdu->rootCost = port->portPriority.rootCost;
    bpdu->swId = port->portPriority.swId;
    //bpdu->swId = sw->swId;
    //bpdu->portId = port->portId;
    bpdu->portId = port->portPriority.txPortId;

    ERROR_Assert(
        port->portTimes.messageAge < port->portTimes.maxAge,
        "SwitchStp_PrepareBpdu: "
        "BPDU to be sent does not message age less than max age.\n");
    bpdu->messageAge =
        SwitchStp_ClocktypeToBpduTime(port->portTimes.messageAge);
    bpdu->maxAge =
        SwitchStp_ClocktypeToBpduTime(port->portTimes.maxAge);
    bpdu->helloTime =
        SwitchStp_ClocktypeToBpduTime(port->portTimes.helloTime);
    bpdu->forwardDelay =
        SwitchStp_ClocktypeToBpduTime(port->portTimes.forwardDelay);
}


// NAME:        SwitchStp_GetBpduRole
//
// PURPOSE:     Determine the role from the flag field in
//              a BPDU. The role is in bits 3 and 4.
//
// PARAMETERS:  flag field of the BPDU
//
// RETURN:      Port role. In practice, the role can either be
//              designated or root.

static
StpPortRole SwitchStp_GetBpduRole(
    unsigned char rxMsgflags)
{
    return (StpPortRole)
        ((rxMsgflags & STP_PORTROLE_MASK) >> STP_PORTROLE_OFFSET);
}


// NAME:        SwitchStp_IsValidBpdu
//
// PURPOSE:     Validate a received BPDU by checking protocol
//              ID, type, size.
//              For Config and RST BPDUs also check if message
//              age is less than max age.
//
//              Reference 9.3.3 and 9.3.4
//
// PARAMETERS:  BPDU message
//
// RETURN:      TRUE if the BPDU passes validation checks.
//
// NOTES:       The validation for loopback conditions has been
//              commented out. See notes within SwitchStp_PrepareBpdu.

static
BOOL SwitchStp_IsValidBpdu(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    Message* bpdu)
{
    StpBpduHdr* hdr;
    int bpduLength;
    BOOL isValidBpdu = TRUE;

    bpduLength = MESSAGE_ReturnPacketSize(bpdu);
    ERROR_Assert(bpduLength >= (signed) (sizeof(StpBpduHdr)),
        "SwitchStp_ValidateBpdu: BPDU size is smaller than a TCN.\n");

    hdr = (StpBpduHdr*) MESSAGE_ReturnPacket(bpdu);
    ERROR_Assert(hdr->protocolId == STP_BPDU_ID,
        "SwitchStp_ValidateBpdu: Unexpected protcol ID.\n");

    if (hdr->protocolVersion == STP_PROTOCOL_VERSION)
    {
        if (hdr->bpduType == STP_BPDU_TYPE)
        {
            StpConfigBpdu* configBpdu =
                (StpConfigBpdu*) MESSAGE_ReturnPacket(bpdu);

            ERROR_Assert(bpduLength == sizeof(StpConfigBpdu),
                "SwitchStp_ValidateBpdu: "
                "Config BPDU has incorrect length.\n");

            //isValidBpdu &=
            //    !(SwitchStp_CompareSwitchId(
            //              sw->swId, configBpdu->swId)
            //        == STP_BOTH_ARE_SAME
            //    && SwitchStp_ComparePortId(
            //              port->portId, configBpdu->portId)
            //        == STP_BOTH_ARE_SAME);

            isValidBpdu &=
                configBpdu->messageAge < configBpdu->maxAge;
        }
        else if (hdr->bpduType == STP_TCN_TYPE)
        {
            ERROR_Assert(bpduLength == sizeof(StpBpduHdr),
                "SwitchStp_ValidateBpdu: "
                "TCN BPDU has incorrect length.\n");
        }
    }
    else if (hdr->protocolVersion == STP_RST_PROTOCOL_VERSION)
    {
        if (hdr->bpduType == STP_RST_TYPE)
        {
            StpRstBpdu* rstBpdu =
                (StpRstBpdu*) MESSAGE_ReturnPacket(bpdu);

            ERROR_Assert(bpduLength == sizeof(StpRstBpdu),
                "SwitchStp_ValidateBpdu: "
                "RST BPDU has incorrect length.\n");

            //isValidBpdu &=
            //    !(SwitchStp_CompareSwitchId(sw->swId, rstBpdu->swId)
            //        == STP_BOTH_ARE_SAME
            //    && SwitchStp_ComparePortId(port->portId, rstBpdu->portId)
            //        == STP_BOTH_ARE_SAME);

            isValidBpdu &=
                rstBpdu->messageAge < rstBpdu->maxAge;
        }
        else
        {
            ERROR_ReportError(
                "SwitchStp_ValidateBpdu: Unknown RST BPDU.\n");
        }
    }
    else
    {
        ERROR_ReportError(
            "SwitchStp_ValidateBpdu: "
            "Unsupported BPDU protocol version.\n");
    }

    return isValidBpdu;
}


// ------------------------------------------------------------------
// STP trace


// The STP.trace file may be optionally generated by adding
// SWITCH-TRACE to the user configuration file. It contains
// details of BPDU exchange and, additionally, comments for
// exception events.
//
// The file uses an ad-hoc ascii format as described in
// SwitchStp_TraceWriteFileFormatDescription.
//
// The trace file may additionally contain comments for
// exception events
// e.g. Drop, Exceeds max age
//
// All trace calls are to SwitchStp_Trace. Refer to that function
// for more details.



// Control comments output to the trace file
#define STP_TRACE_COMMENTS      0

// Control whether switch/port states are traced
#define STP_TRACE_PORT_STATES   0

// Control whether switch/port variables are traced
#define STP_TRACE_VARS          0

// Control whether filter database messages are traced
#define STP_TRACE_FILTER_DB     0

// BPDU type strings are CFG, TCN, RST, and ??? for unknown
#define STP_TRACE_BPDU_TYPE_STRING_LENGTH   4

// Port roles strings are Unkwn, AltBk, Root, Desig, Disab, ?????
#define STP_TRACE_PORT_ROLE_STRING_LENGTH   6


// Write the header of the trace file describing output fields
static
void SwitchStp_TraceWriteFileFormatDescription(
    FILE* fp)
{
    fprintf(fp,
        "File: switch.trace\n"
        "\n"
        "Fields are space separated.\n"
        "Switch IDs are - separated e.g. priority-switchNumber.\n"
        "Path cost is a number.\n"
        "Port IDs are : separated e.g. priority:portNumber.\n"
        "Times are to the nearest millisecond.\n"
        "Flags acronyms are.\n"
        "    TA - Topology change ack\n"
        "    AG - Agreement flag\n"
        "    FW - Forwarding flag\n"
        "    LR - Learning flag\n"
        "    Unkwn/AltBk/Root/Desig/Disab - Port Role\n"
        "    PR - Proposal flag\n"
        "    TC - Topology change\n"
        "\n"
        "The format of BPDUs is:\n"
        "1.  Running serial number (for cross-reference)\n"
        "2.  Switch number of trace capture (may be node ID)\n"
        "3.  Port number of trace capture\n"
        "4.  A character indicating S)end R)eceive\n"
        "5.  Type of BPDU (RST, CFG, TCN)\n"
        "6.  Time, secs (there may be Q delay for a Send)\n"
        "    --- (separator)\n"
        "    Fields as necessary (depending on BPDU type)\n"
        "    RST: [root ID, root cost, switch ID, port ID]\n"
        "         [msgAge, maxAge, helloTime, fwdDelay]\n"
        "         [flags]\n"
        "    STP: Same as RST\n"
        "         Only the TA and TC flags are used\n"
        "    TCN: nothing\n");

    if (STP_TRACE_COMMENTS)
    {
        fprintf(fp,
            "\n"
            "Additional comments are of the form:\n"
            "    SwitchNumber, <PortNumber or 0> -- free form comment\n");
    }

    fprintf(fp, "\n");
}


// Return the BPDU type as a string
static
void SwitchStp_TraceGetBpduTypeAsString(
    StpBpduType bpduType,
    char* const bpduTypeString)
{
    switch (bpduType)
    {

    case STP_BPDUTYPE_CONFIG:
        strcpy(bpduTypeString, "CFG");
        break;

    case STP_BPDUTYPE_TCN:
        strcpy(bpduTypeString, "TCN");
        break;

    case STP_BPDUTYPE_RST:
        strcpy(bpduTypeString, "RST");
        break;

    default:
        strcpy(bpduTypeString, "???");
        break;

    } //switch
}


// Return the port role in the flag field of the BPDU as a string.
static
void SwitchStp_TraceGetPortRoleAsString(
    StpPortRole role,
    char* const roleString)
{
    switch (role)
    {

    case STP_ROLE_UNKNOWN:
        strcpy(roleString, "Unkwn");
        break;

    case STP_ROLE_ALTERNATEORBACKUP:
        strcpy(roleString, "AltBk");
        break;

    case STP_ROLE_ROOT:
        strcpy(roleString, "Root ");
        break;

    case STP_ROLE_DESIGNATED:
        strcpy(roleString, "Desig");
        break;

    case STP_ROLE_DISABLED:
        strcpy(roleString, "Disab");
        break;

    default:
        strcpy(roleString, "?????");
        break;

    } //switch
}


// Write the common fields for all BPDUs -- Config, TCN and RST.
static
void SwitchStp_TraceWriteCommonHdrFields(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    StpBpduHdr* const hdr,
    StpBpduType bpduType,
    const char* flag,
    UInt32 serialNo,
    FILE* const fp)
{
    float simTime = node->getNodeTime()/((float)SECOND);
    char bpduTypeString[STP_TRACE_BPDU_TYPE_STRING_LENGTH];

    fprintf(fp, "%4u) %4u %2u %c ",
        serialNo,
        sw->swNumber,
        port->portNumber,
        *flag);

    SwitchStp_TraceGetBpduTypeAsString(bpduType, bpduTypeString);

    fprintf(fp, "%s %.4f",
        bpduTypeString,
        simTime);
}


// Write the priority and switch number
static
void SwitchStp_TraceWriteSwitchId(
    const StpSwitchId* const swId,
    FILE* const fp)
{

    unsigned int addr = 0;
    char id[4];
    for(int i = 3,j =0;i < MAC_ADDRESS_DEFAULT_LENGTH ;i++,j += 1)
    {
        sprintf(&id[j],"%s",&(swId->ipAddr.byte[i]));
        id[j] = id[j] & 0xff;
    }
    memcpy(&addr,&id,sizeof(NodeAddress));

    fprintf(fp, "%u-%u",
        swId->priority,addr);
}


//Write the port priority and port number
static
void SwitchStp_TraceWritePortId(
    const StpPortId* const portId,
    FILE* const fp)
{
    fprintf(fp, "%u:%u",
        portId->priority,
        portId->portNumber);
}


// Write a time from the BPDU header in seconds format
static
void SwitchStp_TraceWriteBpduTime(
    unsigned short bpduTime,
    FILE* const fp)
{
    fprintf(fp, "%.1f",
        SwitchStp_BpduTimeToClocktype(bpduTime)/(float)SECOND);
}


// Write fields applicable to Config and RST BPDUs.
static
void SwitchStp_TraceWriteBpduMsgFields(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    Message* msg,
    StpBpduType bpduType,
    FILE* fp)
{
    StpConfigBpdu* bpdu = (StpConfigBpdu*) MESSAGE_ReturnPacket(msg);
    char portRoleString[STP_TRACE_PORT_ROLE_STRING_LENGTH];

    fprintf(fp, "[");
    SwitchStp_TraceWriteSwitchId(&bpdu->rootId, fp);
    fprintf(fp, " ");
    fprintf(fp, "%u", bpdu->rootCost);
    fprintf(fp, " ");
    SwitchStp_TraceWriteSwitchId(&bpdu->swId, fp);
    fprintf(fp, " ");
    SwitchStp_TraceWritePortId(&bpdu->portId, fp);
    fprintf(fp, "]");

    fprintf(fp, "[");
    SwitchStp_TraceWriteBpduTime(bpdu->messageAge, fp);
    fprintf(fp, " ");
    SwitchStp_TraceWriteBpduTime(bpdu->maxAge, fp);
    fprintf(fp, " ");
    SwitchStp_TraceWriteBpduTime(bpdu->helloTime, fp);
    fprintf(fp, " ");
    SwitchStp_TraceWriteBpduTime(bpdu->forwardDelay, fp);
    fprintf(fp, "]");

    SwitchStp_TraceGetPortRoleAsString(
        SwitchStp_GetBpduRole(bpdu->flags),
        portRoleString);

    fprintf(fp, "[%s %s %s %s %s %s %s]",
        (bpdu->flags & STP_TC_ACK_FLAG) ? "TA" : "  ",
        (bpdu->flags & STP_AGREEMENT_FLAG) ? "AG" : "  ",
        (bpdu->flags & STP_FORWARDING_FLAG) ? "FW" : "  ",
        (bpdu->flags & STP_LEARNING_FLAG) ? "LR" : "  ",
        portRoleString,
        (bpdu->flags & STP_PROPOSAL_FLAG) ? "PR" : "  ",
        (bpdu->flags & STP_TC_FLAG) ? "TC" : "  ");
}


// Write fields relevant to a Config BPDU.
static
void SwitchStp_TraceWriteConfig(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    Message* msg,
    FILE* fp)
{
    StpConfigBpdu* hdr = (StpConfigBpdu *) MESSAGE_ReturnPacket(msg);

    ERROR_Assert(hdr->bpduHdr.protocolVersion == STP_PROTOCOL_VERSION
        && hdr->bpduHdr.bpduType == STP_BPDU_TYPE,
        "SwitchStp_TraceWriteConfig: Incorrect bpdu header.\n");

    SwitchStp_TraceWriteBpduMsgFields(node, sw, port, msg,
        STP_BPDUTYPE_CONFIG, fp);
}


// Write fields relevant to a TCN.
static
void SwitchStp_TraceWriteTcn(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    Message* msg,
    FILE* fp)
{
    StpBpduHdr* hdr = (StpBpduHdr *) MESSAGE_ReturnPacket(msg);

    ERROR_Assert(hdr->protocolVersion == STP_PROTOCOL_VERSION
        && hdr->bpduType == STP_TCN_TYPE,
        "SwitchStp_TraceWriteTcn: Incorrect bpdu header.\n");

}


// Write fields relevant to a RST BPDU.
static
void SwitchStp_TraceWriteRstp(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    Message* msg,
    FILE* fp)
{
    StpBpduHdr* hdr = (StpBpduHdr *) MESSAGE_ReturnPacket(msg);

    ERROR_Assert(hdr->protocolVersion == STP_RST_PROTOCOL_VERSION
        && hdr->bpduType == STP_RST_TYPE,
        "SwitchStp_TraceWriteRstp: Incorrect bpdu header.\n");

    SwitchStp_TraceWriteBpduMsgFields(node, sw, port, msg,
        STP_BPDUTYPE_RST, fp);
}


#define PRINTLINE     fprintf(fp, \
    "  |                                                             " \
    "       |\n");

#define PRINT_HLINE   fprintf(fp, \
    "  --------------------------------------------------------------" \
    "--------\n");


static
void SwitchStp_TraceWritePriority(
    const char* parameter,
    const StpPriority priorityVar,
    FILE* fp)
{

    unsigned int rootSwitchId = 0;
    unsigned int switchId = 0;
    char rootid[4];
    char switchIdString[4];
    for(int i = 3,j =0;i < MAC_ADDRESS_DEFAULT_LENGTH ;i++,j++)
    {
        sprintf(&rootid[j],"%s",&(priorityVar.rootSwId.ipAddr.byte[i]));
        sprintf(&switchIdString[j],"%s",&(priorityVar.swId.ipAddr.byte[i]));
        rootid[j] = rootid[j] & 0xff;
        switchIdString[j] = switchIdString[j] && 0xff;
    }

    memcpy(&rootSwitchId,&rootid,sizeof(NodeAddress));
    memcpy(&switchId,&switchIdString,sizeof(NodeAddress));


    fprintf(fp,
        "  | %-15s   %5u-%-5x,  %8u,  %5u-%-5x,  %3u:%-4u  |\n",
        parameter,
        priorityVar.rootSwId.priority,
        rootSwitchId,
        priorityVar.rootCost,
        priorityVar.swId.priority,
        switchId,
        priorityVar.txPortId.priority,
        priorityVar.txPortId.portNumber);
}


static
float SwitchStp_TraceReturnTimeInFloat(
    clocktype time)
{
    return (float) (time / (float) SECOND);
}


static
void SwitchStp_TraceWriteTimes(
    const char* parameter,
    const StpTimes timeVar,
    FILE* fp)
{
    fprintf(fp,
        "  | %-15s     %8.2f,   %8.2f,    %8.2f,   %8.2f  |\n",
        parameter,
        SwitchStp_TraceReturnTimeInFloat(timeVar.messageAge),
        SwitchStp_TraceReturnTimeInFloat(timeVar.maxAge),
        SwitchStp_TraceReturnTimeInFloat(timeVar.helloTime),
        SwitchStp_TraceReturnTimeInFloat(timeVar.forwardDelay));
}


static
void SwitchStp_TraceWriteRoleVal(
    StpPortRole role,
    FILE* fp)
{
    switch (role)
    {
    case STP_ROLE_UNKNOWN:
        fprintf(fp, "%s ", "Unkwn");
        break;
    case STP_ROLE_ALTERNATEORBACKUP:
        fprintf(fp, "%s ", "AltBk");
        break;
    case STP_ROLE_ROOT:
        fprintf(fp, "%s ", "Root ");
        break;
    case STP_ROLE_DESIGNATED:
        fprintf(fp, "%s ", "Desig");
        break;
    case STP_ROLE_DISABLED:
        fprintf(fp, "%s ", "Disab");
        break;
    default:
        fprintf(fp, "%s ", "?????");
        break;
    } //switch
}

static
void SwitchStp_TraceWriteVars(
    Node* node,
    MacSwitch* sw,
    FILE* fp)
{
    SwitchPort* port;


//  ----------------------------------------------------------------------
//  | swNumber         T   | swPriority       T   | rootPort             |
//  |                      |                      |                      |
//  | rootPriority                    | rootTimes                        |
//  | swPriority                      | swTimes                          |
//  |                      |                      |                      |


    fprintf(fp, "\n");
    PRINT_HLINE
    fprintf(fp,
        "  |"
        " swNumber      %4u   |"
        " swPriority    %4u   |"
        " rootPort   %4u:%-3u |\n",
        sw->swNumber,
        sw->priority,
        sw->rootPortId.priority,
        sw->rootPortId.portNumber);
    PRINTLINE;

    SwitchStp_TraceWritePriority("rootPriority", sw->rootPriority, fp);
    SwitchStp_TraceWriteTimes("rootTimes", sw->rootTimes, fp);

    SwitchStp_TraceWritePriority("swPriority", sw->swPriority, fp);
    SwitchStp_TraceWriteTimes("swTimes", sw->swTimes, fp);
    PRINTLINE;

    port = sw->firstPort;
    while (port != NULL)
    {

//  ----------------------------------------------------------------------
//  | portNumber       T   | pathCost         T   | portPriority         |
//  | macOperational   T   | operP2P          T   | operEdge         T   |
//  |                      |                      |                      |

        PRINT_HLINE

        fprintf(fp,
            "  |"
            " portNumber   %5u   |"
            " pathCost %9u   |"
            " portPriority %5u   |\n",
            port->portNumber,
            port->pathCost,
            port->priority);

        fprintf(fp,
            "  |"
            " macOperational   %s   |"
            " operP2P          %s   |"
            " operEdge         %s   |\n",
            port->macOperational ? "T" : "F",
            port->operPointToPointMac ? "T" : "F",
            port->operEdge ? "T" : "F");
        PRINTLINE;


//  | rootPriority                    | rootTimes                        |
//  | desigPriority                   | desigTimes                       |
//  |                      |                      |                      |

        SwitchStp_TraceWritePriority(
            "desigPriority", port->designatedPriority, fp);
        SwitchStp_TraceWriteTimes(
            "desigTimes", port->designatedTimes, fp);
        SwitchStp_TraceWritePriority(
            "portPriority", port->portPriority, fp);
        SwitchStp_TraceWriteTimes(
            "portTimes", port->portTimes, fp);
        PRINTLINE;


//  | role             X   | selectedRole     X   |                  T   |
//  | infoIs           X   | rcvdMsg          X   |                  T   |
//  |                      |                      |                      |


        fprintf(fp,
            "  |"
            " role         ");
        SwitchStp_TraceWriteRoleVal(port->role, fp),
        fprintf(fp, "  | selectedRole ");
        SwitchStp_TraceWriteRoleVal(port->selectedRole, fp),
        fprintf(fp, "  |                      |\n");

        PRINTLINE;

//  | fdWhile          T   | helloWhen        T   | mdelayWhile      T   |
//  | rbWhile          T   | rcvdInfoWhile    T   | rrWhile          T   |
//  | tcWhile          T   | txCount          T   |                  T   |
//  |                      |                      |                      |

        fprintf(fp,
            "  |"
            " fdWhile       %6.2f |"
            " helloWhen     %6.2f |"
            " mdelayWhile   %6.2f |\n"
            "  |"
            " rbWhile       %6.2f |"
            " rcvdInfoWhile %6.2f |"
            " rrWhile       %6.2f |\n"
            "  |"
            " tcWhile       %6.2f |"
            " txCount      %7u |"
            "                      |\n",
            SwitchStp_TraceReturnTimeInFloat(port->fdWhile),
            SwitchStp_TraceReturnTimeInFloat(port->helloWhen),
            SwitchStp_TraceReturnTimeInFloat(port->mdelayWhile),
            SwitchStp_TraceReturnTimeInFloat(port->rbWhile),
            SwitchStp_TraceReturnTimeInFloat(port->rcvdInfoWhile),
            SwitchStp_TraceReturnTimeInFloat(port->rrWhile),
            SwitchStp_TraceReturnTimeInFloat(port->tcWhile),
            port->txCount);
        PRINTLINE;

//  | agreed           T   | forward          T   | forwarding       T   |
//  | initPm           T   | learn            T   | learning         T   |
//  | mcheck           T   | newInfo          T   | proposed         T   |
//  | proposing        T   | reRoot           T   | reselect         T   |
//  | selected         T   | sync             T   | synced           T   |
//  | tc               T   | tcAck            T   | tcProp           T   |
//  | updtInfo         T   |                  T   |                  T   |
//  |                      |                      |                      |

        fprintf(fp,
            "  |"
            " agreed           %s  |"
            " forward          %s  |"
            " forwarding       %s  |\n"
            "  |"
            " initPm           %s  |"
            " learn            %s  |"
            " learning         %s  |\n"
            "  |"
            " mcheck           %s  |"
            " newInfo          %s  |"
            " proposed         %s  |\n"
            "  |"
            " proposing        %s  |"
            " reRoot           %s  |"
            " reselect         %s  |\n"
            "  |"
            " selected         %s  |"
            " sync             %s  |"
            " synced           %s  |\n"
            "  |"
            " tc               %s  |"
            " tcAck            %s  |"
            " tcProp           %s  |\n"
            "  |"
            " updtInfo         %s  |"
            "                      |"
            "                      |\n",
            port->agreed ? "T " : "F ",
            port->forward ? "T " : "F ",
            port->forwarding ? "T " : "F ",
            port->initPm ? "T " : "F ",
            port->learn ? "T " : "F ",
            port->learning ? "T " : "F ",
            port->mcheck ? "T " : "F ",
            port->newInfo ? "T " : "F ",
            port->proposed ? "T " : "F ",
            port->proposing ? "T " : "F ",
            port->reRoot ? "T " : "F ",
            port->reselect ? "T " : "F ",
            port->selected ? "T " : "F ",
            port->sync ? "T " : "F ",
            port->synced ? "T " : "F ",
            port->tc ? "T " : "F ",
            port->tcAck ? "T " : "F ",
            port->tcProp ? "T " : "F ",
            port->updtInfo ? "T " : "F ");
        PRINTLINE;
        fprintf(fp, "\n");

        port = port->nextPort;
    }
}


static
void SwitchStp_WriteFilterDbEntryType(
    SwitchDbEntryType entryType,
    FILE* fp)
{
    switch (entryType)
    {
    case SWITCH_DB_DEFAULT:
        fprintf(fp, "%s", "Default\n");
        break;
    case SWITCH_DB_STATIC:
        fprintf(fp, "%s", "Static \n");
        break;
    case SWITCH_DB_CONFIGURED:
        fprintf(fp, "%s", "Config \n");
        break;
    case SWITCH_DB_DYNAMIC:
        fprintf(fp, "%s", "Dynamic\n");
        break;
    default :
        fprintf(fp, "%s", "???????\n");
    } // switch
}


// Format is
//    Switch: number
//                 PortNum  State  LR FW
static
void SwitchStp_TracePortStates(
    Node* node,
    MacSwitch* sw,
    FILE* fp)
{
    SwitchPort* port = sw->firstPort;
    char portRoleString[STP_TRACE_PORT_ROLE_STRING_LENGTH];

    fprintf(fp, "Switch: %d\n", sw->swNumber);

    while (port != NULL)
    {
        SwitchStp_TraceGetPortRoleAsString(port->role, portRoleString),
        fprintf(fp, "    %d - %s   %s  %s\n",
            port->portNumber,
            portRoleString,
            port->learning ? "LR" : "  ",
            port->forwarding ? "FW" : "  ");
        port = port->nextPort;
    }
}


// NAME:        SwitchStp_Trace
//
// PURPOSE:     Common entry routine for trace.
//              Creates and writes to a file called "switch.trace".
//              Output is an internal ascii format with values of
//              interest described in the function
//              SwitchStp_TraceWriteFileFormatDescription.
//
//              Besides BPDU and TCN specific fields, comments may
//              be written to trace by setting the flag parameter
//              to any value except "Send" or "Receive" (other key
//              values subsequently introduced are "FlDb", "Vars",
//              "PortStates"). In such cases, the message parameter,
//              Message* msg, is ignored and can be NULL. Also, if
//              port value is unimportant, it may be set to NULL.
//
// PARAMETERS:  Frame message
//              Flag which is either "Send" or "Receive" or comment
//
// RETURN:      None

void SwitchStp_Trace(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    Message *msg,
    const char* flag)
{
    static UInt32 count=0;
    FILE *fp;
    StpBpduHdr* hdr;
    StpBpduType bpduType;

    if (!sw->stpTrace)
    {
        return;
    }

    if (count == 0)
    {
        fp = fopen("switch.trace", "w");
        ERROR_Assert(fp != NULL,
            "SwitchStp_Trace: file initial open error.\n");
        SwitchStp_TraceWriteFileFormatDescription(fp);
    }
    else
    {
        fp = fopen("switch.trace", "a");
        ERROR_Assert(fp != NULL,
            "SwitchStp_Trace: file append error.\n");
    }


    if (STP_TRACE_FILTER_DB
        && !strncmp( "Fldb:", flag, 5))
    {
        char currentTime[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(node->getNodeTime(), currentTime);
        fprintf(fp,
            "  Switch %d, Port %d at %s -- %s\n",
            sw->swNumber,
            port != NULL ? port->portNumber : 0,
            currentTime,
            flag);

        fclose(fp);
        return;
    }

    // Trace port states for the switch
    if (!strcmp(flag, "PortStates"))
    {
        if (STP_TRACE_PORT_STATES)
        {
            SwitchStp_TracePortStates(node, sw, fp);
        }
        fclose(fp);
        return;
    }

    // Trace switch and port variables
    if (!strcmp(flag, "Vars"))
    {
        if (STP_TRACE_VARS)
        {
            SwitchStp_TraceWriteVars(node, sw, fp);
        }
        fclose(fp);
        return;
    }

    // Write any comment that is passed as parameter
    if (strcmp(flag, "Send")
        && strcmp(flag, "Receive"))
    {
        if (STP_TRACE_COMMENTS)
        {
            fprintf(fp, "Switch %d, Port %d -- %s\n",
                sw->swNumber,
                port != NULL ? port->portNumber : 0,
                flag);
        }
        fclose(fp);
        return;
    }

    count++;

    // Write BPDUs received or sent
    if (!strcmp(flag, "Send")
        || !strcmp(flag, "Receive"))
    {
        hdr = (StpBpduHdr*) MESSAGE_ReturnPacket(msg);
        bpduType = SwitchStp_GetBpduType(msg);

        SwitchStp_TraceWriteCommonHdrFields(node, sw, port,
            hdr, bpduType, flag, count, fp);
        fprintf(fp, " --- ");

        switch (bpduType)
        {

        case STP_BPDUTYPE_CONFIG:
            SwitchStp_TraceWriteConfig(node, sw, port, msg, fp);
            break;

        case STP_BPDUTYPE_TCN:
            SwitchStp_TraceWriteTcn(node, sw, port, msg, fp);
            break;

        case STP_BPDUTYPE_RST:
            SwitchStp_TraceWriteRstp(node, sw, port, msg, fp);
            break;

        default:
            ERROR_ReportError(
                "SwitchStp_Trace: Unrecognized BPDU.\n");
            break;

        } //switch
    }

    fprintf(fp, "\n");

    fclose(fp);
}


// NAME:        SwitchStp_TraceInit
//
// PURPOSE:     Initialize trace setting from user configuration file
//
// PARAMETERS:
//
// RETURN:      None

void SwitchStp_TraceInit(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    char yesOrNo[MAX_STRING_LENGTH];
    BOOL retVal;

    // Initialize trace values for the node
    // <TraceType> is one of
    //      NO    the default if nothing is specified
    //      YES   an ascii format
    // Format is: SWITCH-TRACE <TraceType>

    IO_ReadString(
         node->nodeId,
         ANY_ADDRESS,
         nodeInput,
         "SWITCH-TRACE",
         &retVal,
         yesOrNo);

    if (retVal == TRUE)
    {
        if (!strcmp(yesOrNo, "NO"))
        {
            sw->stpTrace = FALSE;
        }
        else if (!strcmp(yesOrNo, "YES"))
        {
            sw->stpTrace = TRUE;
        }
        else
        {
            ERROR_ReportError(
                "SwitchStp_TraceInit: "
                "Unknown value of SWITCH-TRACE in configuration file.\n"
                "Expecting YES or NO\n");
        }
    }
    else
    {
        sw->stpTrace = SWITCH_TRACE_DEFAULT;
    }
}


// ------------------------------------------------------------------
// STP related functions


// NAME:        SwitchStp_ClearReselectBridge
//
// PURPOSE:     Clear reselect for all ports once port
//              role selection is initiated.
//
//              Ref: 17.19.1
//
// PARAMETERS:  None
//
// RETURN:      None

static
void SwitchStp_ClearReselectBridge(
    Node* node,
    MacSwitch* sw)
{
    SwitchPort* port = sw->firstPort;

    while (port != NULL)
    {
        port->reselect = FALSE;
        port = port->nextPort;
    }
}


// NAME:        SwitchStp_DisableForwarding
//
// PURPOSE:     Clear queues and learnt dynamic database entries
//              when a port's forwarding function is disabled.
//
//              Note: the packet in the port's protocol specific
//              buffer is not cleared.
//
//              Ref: 17.19.2
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_DisableForwarding(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    SwitchDb_DeleteEntriesByPortAndType(
        sw->db, port->portNumber, SWITCH_DB_DYNAMIC, node->getNodeTime());

    Switch_ClearMessagesFromQueue(node, port->outPortScheduler);

    ERROR_Assert(
        SwitchDb_GetCountForPort(
            sw->db, port->portNumber, SWITCH_DB_DYNAMIC) == 0,
        "SwitchStp_DisableLearning: "
        "Port's dynamic entry count is non-zero.\n");

    ERROR_Assert(Switch_QueueIsEmpty(port->outPortScheduler),
        "SwitchStp_DisableLearning: Port queue is not empty.\n");

    SwitchStp_Trace(node, sw, port, NULL,
        "Disabling forwarding, port database & queues flushed");

}


// NAME:        SwitchStp_DisableLearning
//
// PURPOSE:     Clear learnt dynamic database entries when a
//              port's learning function is disabled. There
//              is no need to flush the queues; they should be
//              empty.
//
//              Ref: 17.19.3
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_DisableLearning(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    SwitchDb_DeleteEntriesByPortAndType(
        sw->db, port->portNumber, SWITCH_DB_DYNAMIC, node->getNodeTime());

    ERROR_Assert(
        SwitchDb_GetCountForPort(
            sw->db, port->portNumber, SWITCH_DB_DYNAMIC) == 0,
        "SwitchStp_DisableLearning: "
        "Port's dynamic entry count is non-zero.\n");

    SwitchStp_Trace(node, sw, port, NULL,
        "Disabling learning, port database flushed");

    SwitchPort_DisableForwarding(node, port);
}


// NAME:        SwitchStp_EnableForwarding
//
// PURPOSE:     Taken actions to enable forwarding. This change
//              could be directly from the disabled state.
//              Since database, queues are setup during
//              initialization, no related action is required.
//
//              Ref: 17.19.4
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_EnableForwarding(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    SwitchStp_Trace(node, sw, port, NULL,
        "Enabling forwarding.");

    SwitchPort_EnableForwarding(node, port);
}


// NAME:        SwitchStp_EnableLearning
//
// PURPOSE:     Initiate actions to enable learning. The state
//              change should be from the disabled state.
//
//              Ref: 17.19.5
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_EnableLearning(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    ERROR_Assert(
        SwitchDb_GetCountForPort(
            sw->db, port->portNumber, SWITCH_DB_DYNAMIC) == 0,
        "SwitchStp_EnableLearning: "
        "Port's dynamic entry count is non-zero.\n");

    SwitchStp_Trace(node, sw, port, NULL,
        "Enabling learning");

}


// NAME:        SwitchStp_FlushDatabase
//
// PURPOSE:     Flush all dynamic entries learnt on a port.
//
//              Ref: 17.19.6
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_FlushDatabase(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    if (!port->operEdge
        || sw->forceVersion == 0)    // 802.1y change for forceVersion
    {
        SwitchDb_DeleteEntriesByPortAndType(
            sw->db, port->portNumber, SWITCH_DB_DYNAMIC, node->getNodeTime());

        ERROR_Assert(
            SwitchDb_GetCountForPort(
                sw->db, port->portNumber, SWITCH_DB_DYNAMIC) == 0,
            "SwitchStp_FlushDatabase: "
            "Port's dynamic entry count is non-zero.\n");

        SwitchStp_Trace(node, sw, port, NULL, "Port DB entries flushed");
    }
}


// NAME:        SwitchStp_NewTcWhile
//
// PURPOSE:     Determine the value for topology change timer.
//              The value is either twice hello time for p2p
//              links or the sum of max age + forward delay.
//
//              Ref: 17.19.7
//
// PARAMETERS:
//
// RETURN:      calculated value of timer as clocktype

static
clocktype SwitchStp_NewTcWhile(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    clocktype newTcWhile;

    if (port->sendRstp
        && port->operPointToPointMac)
    {
        newTcWhile = 2 * sw->rootTimes.helloTime;
    }
    else
    {
        newTcWhile = sw->rootTimes.maxAge + sw->rootTimes.forwardDelay;
    }

    return newTcWhile;
}


// NAME:        SwitchStp_CompareRcvdBpdu
//
// PURPOSE:     Compare the received BPDU with the port vector
//              to determine if it is a superior, repeated,
//              confirmed or other type.
//
//              Ref: 17.19.8
//
// PARAMETERS:
//
// RETURN:      One of StpReceivedMsgType types

static
StpReceivedMsgType SwitchStp_CompareRcvdBpdu(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    StpComparisonType compareMsg = SwitchStp_ComparePriority(
        &port->msgPriority, &port->portPriority);
    StpComparisonType compareTimes;

    if ((port->msgType == STP_BPDUTYPE_RST
            && SwitchStp_GetBpduRole(port->msgFlags) == STP_ROLE_DESIGNATED)
        || port->msgType == STP_BPDUTYPE_CONFIG)
    {
        compareTimes = SwitchStp_CompareTimes(
            &port->msgTimes, &port->portTimes);

        if (compareMsg == STP_FIRST_IS_BETTER)
        {
            return STP_SUPERIOR_DESIGNATED_MSG;
        }
        else
        if ((SwitchStp_CompareSwitchId(
                &(port->msgPriority.swId),
                &(port->portPriority.swId))
                == STP_BOTH_ARE_SAME
            && SwitchStp_ComparePortId(
                &(port->msgPriority.txPortId),
                &(port->portPriority.txPortId))
                == STP_BOTH_ARE_SAME)
            && (compareMsg != STP_BOTH_ARE_SAME
                || compareTimes != STP_BOTH_ARE_SAME))
        {
            return STP_SUPERIOR_DESIGNATED_MSG;
        }

        if (compareMsg == STP_BOTH_ARE_SAME
            && compareTimes == STP_BOTH_ARE_SAME)
        {
            return STP_REPEATED_DESIGNATED_MSG;
        }
    }

    if (port->operPointToPointMac
        && port->msgType == STP_BPDUTYPE_RST
        && SwitchStp_GetBpduRole(port->msgFlags) == STP_ROLE_ROOT
        && port->msgFlags & STP_AGREEMENT_FLAG
        && compareMsg == STP_BOTH_ARE_SAME)
    {
        return STP_CONFIRMED_ROOT_MSG;
    }

    return STP_OTHER_MSG;
}


// NAME:        SwitchStp_RecordProposed
//
// PURPOSE:     Determine if proposed should be recorded.
//              The proposed flag is significant in a BPDU
//              received from a designated port on a p2p link.
//
//              Ref: 17.19.9
//
// PARAMETERS:
//
// RETURN:      TRUE if proposed should be recorded.

static
BOOL SwitchStp_RecordProposed(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    if (port->operPointToPointMac
        && port->msgType == STP_BPDUTYPE_RST
        && SwitchStp_GetBpduRole(port->msgFlags) == STP_ROLE_DESIGNATED
        && port->msgFlags & STP_PROPOSAL_FLAG)
    {
        return TRUE;
    }

    return FALSE;
}


// NAME:        SwitchStp_SetSyncBridge
//
// PURPOSE:     Utility function to set the sync flag for all ports.
//
//              Ref: 17.19.10
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_SetSyncBridge(
    Node* node,
    MacSwitch* sw)
{
    SwitchPort* port = sw->firstPort;

    while (port != NULL)
    {
        port->sync = TRUE;
        port = port->nextPort;
    }
}


// NAME:        SwitchStp_SetReRootBridge
//
// PURPOSE:     Utility functions to set reRoot for all ports.
//
//              Ref: 17.19.11
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_SetReRootBridge(
    Node* node,
    MacSwitch* sw)
{
    SwitchPort* port = sw->firstPort;

    while (port != NULL)
    {
        port->reRoot = TRUE;
        port = port->nextPort;
    }
}


// NAME:        SwitchStp_SetSelectedBridge
//
// PURPOSE:     Utility function that sets the reselect flag for
//              all ports if it not set for any port.
//
//              Ref: 17.19.12
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_SetSelectedBridge(
    Node* node,
    MacSwitch* sw)
{
    SwitchPort* port = sw->firstPort;

    while (port != NULL)
    {
        if (port->reselect)
        {

            return;
        }
        port = port->nextPort;
    }

    port = sw->firstPort;
    while (port != NULL)
    {
        port->selected = TRUE;
        port = port->nextPort;
    }
}


// NAME:        SwitchStp_SetTcFlags
//
// PURPOSE:     Set the topology change flags of a port if a
//              received BPDU indicates a TC or acks a TC.
//
//              Ref: 17.19.13
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_SetTcFlags(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    if (port->msgType == STP_BPDUTYPE_RST
        || port->msgType == STP_BPDUTYPE_CONFIG)
    {
        if (port->msgFlags & STP_TC_FLAG)
        {
            port->rcvdTc = TRUE;
        }
        if (port->msgFlags & STP_TC_ACK_FLAG)
        {
            port->rcvdTcAck = TRUE;
        }
    }
    else if (port->msgType == STP_BPDUTYPE_TCN)
    {
        port->rcvdTc = TRUE;
    }
}


// NAME:        SwitchStp_SetTcPropBridge
//
// PURPOSE:     Set the topology change propagation flag of all
//              other ports.
//
//              Ref: 17.19.14
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_SetTcPropBridge(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    SwitchPort* aPort = sw->firstPort;

    while (aPort != NULL)
    {
        if (aPort != port)
        {
            aPort->tcProp = TRUE;
        }
        aPort = aPort->nextPort;
    }
}


// NAME:        SwitchStp_TransmitConfigBpdu
//
// PURPOSE:     Transmit a Config BPDU.
//
//              Ref: 17.19.15
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_TransmitConfigBpdu(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    Message* bpduMsg;
    StpConfigBpdu* bpdu;


    Mac802Address destAddr;
    MAC_CopyMac802Address(&destAddr, &sw->Switch_stp_address);
    Mac802Address sourceAddr;
    MAC_CopyMac802Address(&sourceAddr, &port->portAddr);

    unsigned char flags = 0;
    BOOL isEnqueued;

    bpduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, bpduMsg, sizeof(StpConfigBpdu), TRACE_STP);

    bpdu = (StpConfigBpdu*) MESSAGE_ReturnPacket(bpduMsg);
    memset(bpdu, 0, sizeof(StpConfigBpdu));

    SwitchStp_PrepareBpdu(node, sw, port, bpdu);

    bpdu->bpduHdr.protocolVersion = STP_PROTOCOL_VERSION;
    bpdu->bpduHdr.bpduType = STP_BPDU_TYPE;

    if (port->tcWhile != 0)
    {
        flags |= STP_TC_FLAG;
    }
    if (port->tcAck)
    {
        flags |= STP_TC_ACK_FLAG;
    }
    bpdu->flags = flags;

    Switch_SendFrameToMac(
        node, bpduMsg, CPU_INTERFACE, port->macData->interfaceIndex,
        destAddr, sourceAddr, SWITCH_VLAN_ID_STP,
        SWITCH_FRAME_PRIORITY_MAX, &isEnqueued);

    if (isEnqueued)
    {
        port->stats.configBpdusSent++;
    }
    else
    {
        SwitchStp_Trace(node, sw, port, NULL,
            "Unable to send config BPDU; Q is full.");
        MESSAGE_Free(node, bpduMsg);
    }
}


// NAME:        SwitchStp_TransmitRstBpdu
//
// PURPOSE:     Transmit an RST BPDU.
//
//              Ref: 17.19.16
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_TransmitRstBpdu(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    Message* bpduMsg;
    StpRstBpdu* bpdu;
    Mac802Address destAddr;
    MAC_CopyMac802Address(&destAddr, &sw->Switch_stp_address);
    Mac802Address sourceAddr;
    MAC_CopyMac802Address(&sourceAddr, &port->portAddr);

    unsigned char flags = 0;
    BOOL isEnqueued;

    bpduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, bpduMsg, sizeof(StpRstBpdu), TRACE_STP);

    bpdu = (StpRstBpdu*) MESSAGE_ReturnPacket(bpduMsg);
    memset(bpdu, 0, sizeof(StpRstBpdu));

    SwitchStp_PrepareBpdu(node, sw, port, (StpConfigBpdu*) bpdu);

    bpdu->bpduHdr.protocolVersion = STP_RST_PROTOCOL_VERSION;
    bpdu->bpduHdr.bpduType = STP_RST_TYPE;

    if (port->tcWhile != 0)
    {
        flags |= STP_TC_FLAG;
    }
    if (port->proposing)
    {
        flags |= STP_PROPOSAL_FLAG;
    }

    ERROR_Assert(port->selectedRole != STP_ROLE_DISABLED,
        "SwitchStp_txRstp: Port role is DISABLED..\n");
    flags |= (port->selectedRole << STP_PORTROLE_OFFSET)
        & STP_PORTROLE_MASK;

    // There is no reference to filling in the learning
    // and forwarding flags in the standard. These flags
    // are not used by the receiving switch and are filled
    // in for trace.
    //
    if (port->learning)
    {
        flags |= STP_LEARNING_FLAG;
    }
    if (port->forwarding)
    {
        flags |= STP_FORWARDING_FLAG;
    }

    if (port->synced)
    {
        flags |= STP_AGREEMENT_FLAG;
    }
    bpdu->flags = flags;

    bpdu->version1Length = STP_RST_VERSION1_LENGTH;

    SwitchStp_Trace(node, sw, port, bpduMsg, "Send");

    Switch_SendFrameToMac(
        node, bpduMsg, CPU_INTERFACE, port->macData->interfaceIndex,
        destAddr, sourceAddr, SWITCH_VLAN_ID_STP,
        SWITCH_FRAME_PRIORITY_MAX, &isEnqueued);

    if (isEnqueued)
    {
        port->stats.rstpBpdusSent++;
    }
    else
    {
        SwitchStp_Trace(node, sw, port, NULL,
            "Unable to send RST BPDU; Q is full.");
        MESSAGE_Free(node, bpduMsg);
    }
}


// NAME:        SwitchStp_TransmitTcnBpdu
//
// PURPOSE:     Transmit a TCN BPDU
//
//              Ref: 17.19.17
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_TransmitTcnBpdu(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    Message* bpduMsg;
    StpBpduHdr* bpduHdr;

    Mac802Address destAddr;
    Mac802Address sourceAddr;
    MAC_CopyMac802Address(&destAddr, &sw->Switch_stp_address);
    MAC_CopyMac802Address(&sourceAddr, &port->portAddr);

    BOOL isEnqueued;

    bpduMsg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node, bpduMsg, sizeof(StpBpduHdr), TRACE_STP);

    bpduHdr = (StpBpduHdr*) MESSAGE_ReturnPacket(bpduMsg);
    memset(bpduHdr, 0, sizeof(StpBpduHdr));

    bpduHdr->protocolId = STP_BPDU_ID;

    bpduHdr->protocolVersion = STP_PROTOCOL_VERSION;
    bpduHdr->bpduType = (unsigned char) STP_TCN_TYPE;

    Switch_SendFrameToMac(
        node, bpduMsg, CPU_INTERFACE, port->macData->interfaceIndex,
        destAddr, sourceAddr, SWITCH_VLAN_ID_STP,
        SWITCH_FRAME_PRIORITY_MAX, &isEnqueued);

    if (isEnqueued)
    {
        port->stats.tcnsSent++;
    }
    else
    {
        SwitchStp_Trace(node, sw, port, NULL,
            "Unable to send TCN BPDU; Q is full.");
        MESSAGE_Free(node, bpduMsg);
    }
}


// NAME:        SwitchStp_UpdateBpduVersion
//
// PURPOSE:     Update the BPDU version specific variables
//              on receipt of a legacy BPDU.
//
//              Ref: 17.19.18
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_UpdateBpduVersion(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    if ((port->msgType == STP_BPDUTYPE_CONFIG
            && port->msgProtocolVersion < STP_RST_PROTOCOL_VERSION)
        || port->msgType == STP_TCN_TYPE)
    {
        port->rcvdStp = TRUE;
    }
    else
    if (port->msgType == STP_BPDUTYPE_RST)
    {
        port->rcvdRstp = TRUE;
    }
}


// NAME:        SwitchStp_UpdateRcvdInfoWhile
//
// PURPOSE:     Update the timer that determines how long the
//              received BPDU should be held before it is aged.
//              A ceiling of thrice hello time is applied to a
//              non-aged BPDU.
//
//              Ref: 17.19.19
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_UpdateRcvdInfoWhile(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    clocktype increment =
        SwitchStp_ClocktypeToNearestSecond(port->portTimes.maxAge / 16);
    clocktype effectiveAge =
        port->portTimes.messageAge + MAX(increment, SECOND);

    if (effectiveAge <= port->portTimes.maxAge)
    {
        port->rcvdInfoWhile = MIN(
            3 * port->portTimes.helloTime,
            port->portTimes.maxAge - effectiveAge);
    }
    else
    {
        port->rcvdInfoWhile = 0;
    }
}


// NAME:        SwitchStp_UpdateRoleDisabledBridge
//
// PURPOSE:     Utility function to set all ports roles as disabled.
//
//              Ref: 17.19.20
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_UpdateRoleDisabledBridge(
    Node* node,
    MacSwitch* sw)
{
    SwitchPort* port = sw->firstPort;

    while (port != NULL)
    {
        port->selectedRole = STP_ROLE_DISABLED;
        port = port->nextPort;
    }
}


// NAME:        SwitchStp_UpdateRolesBridge
//
// PURPOSE:     Update the role of all ports of a switch.
//              The switch root priority, times and all the
//              port designated priorities are also determined.
//
//              Ref: 17.19.21
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_UpdateRolesBridge(
    Node* node,
    MacSwitch* sw)
{
    SwitchPort* port;
    StpVector rootVector;
    StpTimes rootTimes;
    StpVector portRootPathVector;
    clocktype msgAgeIncrement;
    UInt32 pathCost;

    // Assign the default switch vector
    SwitchStp_AssignPortId(0, 0, &sw->rootPortId);
    SwitchStp_AssignPriority(sw->swId, 0, sw->swId, sw->rootPortId,
        &(rootVector.stpPriority));
    rootVector.rxPortId.priority = 0;
    rootVector.rxPortId.portNumber = 0;

    rootTimes = sw->swTimes;

    port = sw->firstPort;
    while (port != NULL)
    {
        if (port->portEnabled
            && port->infoIs == STP_INFOIS_RECEIVED)
        {
            // Compute root path priority vector, 17.19.21 a)
            portRootPathVector.stpPriority = port->portPriority;
            portRootPathVector.rxPortId = port->portId;

            // Ensure that the root path cost does not overflow
            pathCost = portRootPathVector.stpPriority.rootCost
                += port->pathCost;
            if (pathCost <= STP_PATH_COST_MAX)
            {
                portRootPathVector.stpPriority.rootCost = pathCost;
            }
            else
            {
                portRootPathVector.stpPriority.rootCost = STP_PATH_COST_MAX;
            }

            // Compute root priority vector, 17.19.21 c)
            if (SwitchStp_CompareSwitchId(
                    &(portRootPathVector.stpPriority.swId),
                    &(sw->swId))
                    != STP_BOTH_ARE_SAME
                && SwitchStp_CompareVector(
                    &portRootPathVector,
                    &rootVector)
                    == STP_FIRST_IS_BETTER)
            {
                rootVector = portRootPathVector;
                rootTimes = port->portTimes;
            }
        }

        port = port->nextPort;
    }

    sw->rootPriority = rootVector.stpPriority;
    sw->rootPortId = rootVector.rxPortId;
    sw->rootTimes = rootTimes;

    if (SwitchStp_ComparePriority(&sw->rootPriority, &sw->swPriority)
        != STP_BOTH_ARE_SAME)
    {
        msgAgeIncrement =
            SwitchStp_ClocktypeToNearestSecond(sw->rootTimes.maxAge / 16);
        sw->rootTimes.messageAge +=
            MAX(msgAgeIncrement, SECOND);
    }

    // assign port roles, 17.19.21 e to l
    port = sw->firstPort;
    while (port != NULL)
    {
        if (port->role != STP_ROLE_DISABLED)
        {
            if (SwitchStp_CompareSwitchId(
                &(sw->rootPriority.rootSwId),
                &(sw->swId))
                == STP_FIRST_IS_BETTER)
            {
                port->designatedPriority = sw->rootPriority;
                port->designatedTimes = sw->rootTimes;
            }
            else
            {
                port->designatedPriority = sw->swPriority;
                port->designatedTimes = sw->swTimes;
            }
            port->designatedPriority.swId = sw->swId;
            port->designatedPriority.txPortId = port->portId;
        }

        switch (port->infoIs)
        {

        case STP_INFOIS_DISABLED:
            port->selectedRole = STP_ROLE_DISABLED;
            break;

        case STP_INFOIS_AGED:
            port->updtInfo = TRUE;
            port->selectedRole = STP_ROLE_DESIGNATED;
            break;

        case STP_INFOIS_MINE:
            port->selectedRole = STP_ROLE_DESIGNATED;
            if (SwitchStp_ComparePriority(&port->portPriority,
                    &port->designatedPriority) != STP_BOTH_ARE_SAME
                || SwitchStp_CompareTimes(&port->designatedTimes,
                    &sw->rootTimes) != STP_BOTH_ARE_SAME)
            {
                port->updtInfo = TRUE;
            }
            break;

        case STP_INFOIS_RECEIVED:
            if (SwitchStp_ComparePortId(&port->portId, &sw->rootPortId)
                == STP_BOTH_ARE_SAME)
            {
                port->selectedRole = STP_ROLE_ROOT;
                port->updtInfo = FALSE;
            }
            else
            {
                if (SwitchStp_ComparePriority(&port->designatedPriority,
                        &port->portPriority) != STP_FIRST_IS_BETTER)
                {
                    if(SwitchStp_CompareSwitchId(
                            &(port->designatedPriority.swId),
                            &(port->designatedPriority.rootSwId))
                            != STP_BOTH_ARE_SAME
                        || SwitchStp_ComparePortId(
                            &(port->designatedPriority.txPortId),
                            &(port->portId))
                            != STP_BOTH_ARE_SAME)
                    {
                        // Alternate
                        port->selectedRole = STP_ROLE_ALTERNATEORBACKUP;
                    }
                    else
                    {
                        // Backup
                        port->selectedRole = STP_ROLE_ALTERNATEORBACKUP;
                    }
                    port->updtInfo = FALSE;
                }
                else
                {
                    //802.1y 17.19.21m
                    port->selectedRole = STP_ROLE_DESIGNATED;
                    port->updtInfo = TRUE;
                }
            }
            break;

        default:
            ERROR_ReportError(
                "SwitchStp_UpdateRolesBridge: Unexpected INFOIS state.\n");
            break;

        } //switch

        port = port->nextPort;
    }
}



// ------------------------------------------------------------------
// State machines

// Each state machine has
//
// A set of states.
// These states are enumed to reflect the reference section
// of the state machine in the 802.1w standard. For example,
// states related to Sec. 17.21, the Port Information machine
// have states beginning from 172101
//
// A CheckStateIsInRange utility function that returns TRUE
// if the current state is valid.
//
// An EnterState function that executes the instructions when
// the state is entered.
//
// A ChangeState function that checks the conditions for state
// transition.
//
// An Iterate function that continues to ChangeState and
// EnterState till no more change takes place.


// The starting state for all state machines
#define STP_SM_STATES 170000


// NAME:        SwitchStpSm_ChangeState
//
// PURPOSE:     Utility function to change the state.
//
// PARAMETERS:  new state to change to.
//
// RETURN:      TRUE if state changed successfully

static
BOOL SwitchStpSm_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchStpSm* sm,
    int newState)
{
    sm->state = newState;
    sm->stateHasChanged = TRUE;

    ERROR_Assert(
        sm->sm_CheckStateIsInRange(node, sw, NULL, sm)
        || newState == STP_SM_STATES,
        "SwitchStpSm_ChangeState: new state is invalid.\n");

    return TRUE;
}


// ..................................................................
// 17.20 The Port Timers state machine

#define STP_SM_PORT_TIMERS_BEGIN (STP_SM_STATES + 2001)
#define STP_SM_PORT_TIMERS_END (2 + STP_SM_PORT_TIMERS_BEGIN)

typedef enum
{
    STP_SM_TICK = STP_SM_PORT_TIMERS_BEGIN,
    STP_SM_ONE_SECOND
} SwitchStpSmPortTimersType;


static
BOOL SwitchStpSm_PortTimers_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_PORT_TIMERS_BEGIN
        && sm->state < STP_SM_PORT_TIMERS_END;
}


static
void SwitchStpSm_PortTimers_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    switch (sm->state)
    {

    case STP_SM_TICK:
        SwitchStp_DecrementTimerBy1Second(&port->helloWhen);
        SwitchStp_DecrementTimerBy1Second(&port->tcWhile);
        SwitchStp_DecrementTimerBy1Second(&port->fdWhile);
        SwitchStp_DecrementTimerBy1Second(&port->rcvdInfoWhile);
        SwitchStp_DecrementTimerBy1Second(&port->rrWhile);
        SwitchStp_DecrementTimerBy1Second(&port->rbWhile);
        SwitchStp_DecrementTimerBy1Second(&port->mdelayWhile);
        if (port->txCount > 0)
        {
            port->txCount--;
        }

        break;

    case STP_SM_ONE_SECOND:
        port->tick = FALSE;

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortTimers_EnterState: Invalid state.\n");

        break;

    } //switch
}


static
BOOL SwitchStpSm_PortTimers_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ONE_SECOND);
    }

    switch (sm->state)
    {

    case STP_SM_TICK:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ONE_SECOND);

        break;

    case STP_SM_ONE_SECOND:
        if (port->tick)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_TICK);
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortTimers_ChangeState: Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_PortTimers_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_PortTimers_ChangeState(node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_PortTimers_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_PortTimers_ChangeState(node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 17.21 The Port Information state machine

#define STP_SM_PORT_INFO_BEGIN (STP_SM_STATES + 2101)
#define STP_SM_PORT_INFO_END (9 + STP_SM_PORT_INFO_BEGIN)

typedef enum
{
    STP_SM_DISABLED = STP_SM_PORT_INFO_BEGIN,
    STP_SM_ENABLED,                            //802.1y
    STP_SM_AGED,
    STP_SM_UPDATE,
    STP_SM_SUPERIOR,
    STP_SM_REPEAT,
    STP_SM_AGREEMENT,
    STP_SM_CURRENT,
    STP_SM_RECEIVE
} SwitchStpSmPortInfoType;


static
BOOL SwitchStpSm_PortInfo_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return (sm->state >= STP_SM_PORT_INFO_BEGIN
                && sm->state < STP_SM_PORT_INFO_END);
}

static
void SwitchStpSm_PortInfo_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    switch (sm->state)
    {

    case STP_SM_DISABLED:
        port->rcvdBpdu = FALSE;
        port->rcvdRstp =  FALSE;
        port->rcvdStp =  FALSE;

        port->portPriority = port->designatedPriority;
        port->portTimes = port->designatedTimes;

        port->updtInfo =  FALSE;
        port->proposing =  FALSE;

        port->agreed =  FALSE;
        port->proposed =  FALSE;

        port->rcvdInfoWhile = 0;
        port->infoIs = STP_INFOIS_DISABLED;
        port->reselect = TRUE;
        port->selected =  FALSE;

        break;

    case STP_SM_ENABLED:
        port->portPriority = port->designatedPriority;
        port->portTimes = port->designatedTimes;

        break;

    case STP_SM_AGED:
        port->infoIs = STP_INFOIS_AGED;
        port->reselect= TRUE;
        port->selected= FALSE;

        break;

    case STP_SM_UPDATE:
        port->portPriority = port->designatedPriority;
        port->portTimes = port->designatedTimes;

        port->updtInfo = FALSE;
        port->agreed = FALSE;
        port->synced = FALSE;
        port->proposed = FALSE;
        port->proposing = FALSE;
        port->infoIs = STP_INFOIS_MINE;
        port->newInfo = TRUE;

        break;

    case STP_SM_SUPERIOR:
        port->portPriority = port->msgPriority;
        port->portTimes = port->msgTimes;

        SwitchStp_UpdateRcvdInfoWhile(node, sw, port);

        //port->agreed = FALSE;     due to 802.1y
        port->proposing = FALSE;
        //port->synced = FALSE;     due to 802.1y

        port->proposed = SwitchStp_RecordProposed(node, sw, port);

        port->infoIs = STP_INFOIS_RECEIVED;
        port->reselect = TRUE;
        port->selected = FALSE;

        break;

    case STP_SM_REPEAT:
        port->proposed = SwitchStp_RecordProposed(node, sw, port);
        SwitchStp_UpdateRcvdInfoWhile(node, sw, port);

        break;

    case STP_SM_AGREEMENT:
        port->agreed = TRUE;
        port->proposing = FALSE;

        break;

    case STP_SM_CURRENT:

        break;

    case STP_SM_RECEIVE:
        port->rcvdMsg = SwitchStp_CompareRcvdBpdu(node, sw, port);
        SwitchStp_UpdateBpduVersion(node, sw, port);
        SwitchStp_SetTcFlags(node, sw, port);
        port->rcvdBpdu = FALSE;

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortInfo_EnterState: Invalid state.\n");

        break;

    } //switch
}


static
BOOL SwitchStpSm_PortInfo_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_DISABLED);
    }
    if (!port->portEnabled && port->infoIs != STP_INFOIS_DISABLED)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_DISABLED);
    }

    switch (sm->state)
    {

    case STP_SM_DISABLED:
        if (port->updtInfo)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_DISABLED);
        }

        if (port->portEnabled
            && port->selected)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ENABLED);
        }

        if (port->rcvdBpdu)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_DISABLED);
        }

        break;

    case STP_SM_ENABLED:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_AGED);

        break;

    case STP_SM_AGED:
        if (port->selected && port->updtInfo)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_UPDATE);
        }

        break;

    case STP_SM_UPDATE:
    case STP_SM_SUPERIOR:
    case STP_SM_REPEAT:
    case STP_SM_AGREEMENT:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_CURRENT);

        break;

    case STP_SM_CURRENT:
        if (port->selected && port->updtInfo)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_UPDATE);
        }

        if (port->infoIs == STP_INFOIS_RECEIVED
            && port->rcvdInfoWhile == 0
            && !port->updtInfo
            && !port->rcvdBpdu)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_AGED);
        }

        if (port->rcvdBpdu
            && !port->updtInfo)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_RECEIVE);
        }

        break;

    case STP_SM_RECEIVE:
        if (port->rcvdMsg == STP_SUPERIOR_DESIGNATED_MSG)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SUPERIOR);
        }

        if (port->rcvdMsg == STP_REPEATED_DESIGNATED_MSG)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_REPEAT);
        }

        if (port->rcvdMsg == STP_CONFIRMED_ROOT_MSG)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_AGREEMENT);
        }

        if (port->rcvdMsg == STP_OTHER_MSG)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_CURRENT);
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortInfo_ChangeState: Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_PortInfo_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_PortInfo_ChangeState(node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_PortInfo_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_PortInfo_ChangeState(node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 17.22 The Port Role Selection state machine

#define STP_SM_PORT_ROLE_SELECTION_BEGIN (STP_SM_STATES + 2201)
#define STP_SM_PORT_ROLE_SELECTION_END \
            (2 + STP_SM_PORT_ROLE_SELECTION_BEGIN)

typedef enum
{
    STP_SM_INIT_BRIDGE = STP_SM_PORT_ROLE_SELECTION_BEGIN,
    STP_SM_ROLE_SELECTION
} SwitchStpSmPortRoleSelectionType;


static
BOOL SwitchStpSm_PortRoleSelection_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_PORT_ROLE_SELECTION_BEGIN
        && sm->state < STP_SM_PORT_ROLE_SELECTION_END;
}

static
void SwitchStpSm_PortRoleSelection_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    switch (sm->state)
    {

    case STP_SM_INIT_BRIDGE:
        SwitchStp_UpdateRoleDisabledBridge(node, sw);

        break;

    case STP_SM_ROLE_SELECTION:
        SwitchStp_ClearReselectBridge(node, sw);
        SwitchStp_UpdateRolesBridge(node, sw);
        SwitchStp_SetSelectedBridge(node, sw);

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortRoleSelection_EnterState: Invalid state.\n");

        break;

    } //switch
}


static
BOOL SwitchStpSm_PortRoleSelection_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    SwitchPort* aPort;

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_INIT_BRIDGE);
    }

    switch (sm->state)
    {

    case STP_SM_INIT_BRIDGE:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ROLE_SELECTION);

        break;

    case STP_SM_ROLE_SELECTION:
        aPort = sw->firstPort;
        while (aPort != NULL)
        {
            if (aPort->reselect)
            {
                return SwitchStpSm_ChangeState(
                    node, sw, sm, STP_SM_ROLE_SELECTION);
            }
            aPort = aPort->nextPort;
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortRoleSelection_ChangeState: Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_PortRoleSelection_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_PortRoleSelection_ChangeState(node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_PortRoleSelection_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_PortRoleSelection_ChangeState(node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 17.23 The Port Role Transitions state machine

#define STP_SM_PORT_ROLE_TRANSITION_BEGIN (STP_SM_STATES + 2301)
#define STP_SM_PORT_ROLE_TRANSITION_END \
            (18 + STP_SM_PORT_ROLE_TRANSITION_BEGIN)

typedef enum
{
    STP_SM_INIT_PORT = STP_SM_PORT_ROLE_TRANSITION_BEGIN,
    STP_SM_BLOCK_PORT,
    STP_SM_BACKUP_PORT,
    STP_SM_BLOCKED_PORT,

    STP_SM_ROOT_PROPOSED,
    STP_SM_ROOT_AGREED,
    STP_SM_REROOT,
    STP_SM_ROOT_FORWARD,
    STP_SM_ROOT_LEARN,
    STP_SM_REROOTED,
    STP_SM_ROOT_PORT,

    STP_SM_DESIGNATED_PROPOSE,
    STP_SM_DESIGNATED_SYNCED,
    STP_SM_DESIGNATED_RETIRED,
    STP_SM_DESIGNATED_FORWARD,
    STP_SM_DESIGNATED_LEARN,
    STP_SM_DESIGNATED_LISTEN,
    STP_SM_DESIGNATED_PORT

} SwitchStpSmPortRoleTransitionType;


static
BOOL SwitchStpSm_PortRoleTransition_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_PORT_ROLE_TRANSITION_BEGIN
        && sm->state < STP_SM_PORT_ROLE_TRANSITION_END;
}

static
void SwitchStpSm_PortRoleTransition_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    switch (sm->state)
    {

    case STP_SM_INIT_PORT:
        port->role = STP_ROLE_DISABLED;
        port->selectedRole = STP_ROLE_DISABLED;
        port->reselect = TRUE;
        port->synced = FALSE;
        port->sync = TRUE;
        port->reRoot = TRUE;
        port->rrWhile = sw->rootTimes.forwardDelay;
        port->fdWhile = sw->rootTimes.forwardDelay;
        port->rbWhile = 0;

        break;

    case STP_SM_BLOCK_PORT:
        port->role = port->selectedRole;
        port->learn = FALSE;
        port->forward = FALSE;

        break;

    case STP_SM_BACKUP_PORT:
        port->rbWhile = 2 * sw->rootTimes.helloTime;

        break;

    case STP_SM_BLOCKED_PORT:
        port->fdWhile = sw->rootTimes.forwardDelay;
        port->synced = TRUE;
        port->rrWhile = 0;
        port->sync = FALSE;
        port->reRoot = FALSE;

        break;

    case STP_SM_ROOT_PROPOSED:
        SwitchStp_SetSyncBridge(node, sw);
        port->proposed = FALSE;

        break;

    case STP_SM_ROOT_AGREED:
        port->proposed = FALSE;
        port->sync = FALSE;
        port->synced = TRUE;
        port->newInfo = TRUE;

        break;

    case STP_SM_REROOT:
        SwitchStp_SetReRootBridge(node, sw);

        break;

    case STP_SM_ROOT_FORWARD:
        port->fdWhile = 0;
        port->forward = TRUE;
        break;

    case STP_SM_ROOT_LEARN:
        port->fdWhile = sw->rootTimes.forwardDelay;
        port->learn = TRUE;
        break;

    case STP_SM_REROOTED:
        port->reRoot = FALSE;
        break;

    case STP_SM_ROOT_PORT:
        port->role = STP_ROLE_ROOT;
        port->rrWhile = sw->rootTimes.forwardDelay;
        break;

    case STP_SM_DESIGNATED_PROPOSE:
        port->proposing = TRUE;
        port->newInfo = TRUE;

        break;

    case STP_SM_DESIGNATED_SYNCED:
        port->rrWhile = 0;
        port->synced = TRUE;
        port->sync = FALSE;

        break;

    case STP_SM_DESIGNATED_RETIRED:
        port->reRoot = FALSE;

        break;

    case STP_SM_DESIGNATED_FORWARD:
        port->forward = TRUE;
        port->fdWhile = 0;

        break;

    case STP_SM_DESIGNATED_LEARN:
        port->learn = TRUE;
        port->fdWhile = sw->rootTimes.forwardDelay;

        break;

    case STP_SM_DESIGNATED_LISTEN:
        port->learn = FALSE;
        port->forward = FALSE;
        port->fdWhile = sw->rootTimes.forwardDelay;

        break;

    case STP_SM_DESIGNATED_PORT:
        port->role = STP_ROLE_DESIGNATED;

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortRoleTransition_EnterState: Invalid state.\n");

        break;

    } //switch
}


// Note: This state machine assumes a distinction between
// Alternate and Backup states.
static
BOOL SwitchStpSm_PortRoleTransition_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL isSelected = port->selected && !port->updtInfo;
    BOOL allSynced;
    BOOL reRooted;
    SwitchPort* aPort;

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_INIT_PORT);
    }
    if (isSelected
        && port->role != port->selectedRole)
    {
        if (port->selectedRole == STP_ROLE_DISABLED
            || port->selectedRole == STP_ROLE_ALTERNATEORBACKUP)
                                    //AlternatePort
            //|| port->selectedRole == STP_ROLE_ALTERNATEORBACKUP)
                                     //BackupPort
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_BLOCK_PORT);
        }

        if (port->selectedRole == STP_ROLE_ROOT)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ROOT_PORT);
        }

        if (port->selectedRole == STP_ROLE_DESIGNATED)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_DESIGNATED_PORT);
        }
    }

    switch (sm->state)
    {

    case STP_SM_INIT_PORT:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_BLOCK_PORT);

        break;

    case STP_SM_BLOCK_PORT:
        if (isSelected
            && !port->learning
            && !port->forwarding)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_BLOCKED_PORT);
        }

        break;

    case STP_SM_BACKUP_PORT:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_BLOCKED_PORT);

        break;

    case STP_SM_BLOCKED_PORT:
        if (!isSelected)
        {
            break;
        }

        if (port->fdWhile != sw->rootTimes.forwardDelay
            || port->sync || port->reRoot || !port->synced)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_BLOCKED_PORT);
        }

        if (port->rbWhile != 2 * sw->rootTimes.helloTime
            && port->role == STP_ROLE_ALTERNATEORBACKUP) // BackupPort
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_BACKUP_PORT);
        }

        break;

    case STP_SM_ROOT_PROPOSED:
    case STP_SM_ROOT_AGREED:
    case STP_SM_REROOT:
    case STP_SM_ROOT_FORWARD:
    case STP_SM_ROOT_LEARN:
    case STP_SM_REROOTED:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ROOT_PORT);

        break;

    case STP_SM_ROOT_PORT:
        if (!isSelected)
        {
            break;
        }

        if (!port->forward && !port->reRoot)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_REROOT);
        }

        allSynced = TRUE;
        aPort = sw->firstPort;
        while (aPort != NULL && allSynced)
        {
            if (aPort != port)
            {
                allSynced = allSynced && aPort->synced;
            }
            aPort = aPort->nextPort;
        }

        if ((port->proposed && allSynced)
            || (!port->synced && allSynced))
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_ROOT_AGREED);
        }

        if (port->proposed && !port->synced)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_ROOT_PROPOSED);
        }

        reRooted = TRUE;
        aPort = sw->firstPort;
        while (aPort != NULL && reRooted)
        {
            if (aPort != port)
            {
                reRooted = reRooted && (aPort->rrWhile == 0);
            }
            aPort = aPort->nextPort;
        }

        if (((port->fdWhile == 0)
              || ((reRooted && port->rbWhile == 0)
              && (sw->forceVersion >=2)))
            && port->learn
            && !port->forward)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_ROOT_FORWARD);
        }

        if (((port->fdWhile == 0)
              || ((reRooted && port->rbWhile == 0)
              && (sw->forceVersion >=2)))
            && !port->learn)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_ROOT_LEARN);
        }

        if (port->reRoot && port->forward)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_REROOTED);
        }

        if (port->rrWhile != sw->rootTimes.forwardDelay)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ROOT_PORT);
        }

        break;

    case STP_SM_DESIGNATED_PROPOSE:
    case STP_SM_DESIGNATED_SYNCED:
    case STP_SM_DESIGNATED_RETIRED:
    case STP_SM_DESIGNATED_FORWARD:
    case STP_SM_DESIGNATED_LEARN:
    case STP_SM_DESIGNATED_LISTEN:
        return SwitchStpSm_ChangeState(
            node, sw, sm, STP_SM_DESIGNATED_PORT);

        break;

    case STP_SM_DESIGNATED_PORT:
        if (!isSelected)
        {
            break;
        }

        if (port->rrWhile == 0 && port->reRoot)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_DESIGNATED_RETIRED);
        }

        if ((!port->learning && !port->forwarding && !port->synced)
            || (port->agreed && !port->synced)
            || (port->operEdge && !port->synced)
            || (port->sync && port->synced))
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_DESIGNATED_SYNCED);
        }

        if (!port->forward && !port->agreed
            && !port->proposing && !port->operEdge)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_DESIGNATED_PROPOSE);
        }

        if ((port->fdWhile == 0 || port->agreed || port->operEdge)
            && (port->rrWhile ==0 || !port->reRoot)
            && !port->sync
            && (port->learn && !port->forward))
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_DESIGNATED_FORWARD);
        }

        if ((port->fdWhile == 0 || port->agreed || port->operEdge)
            && (port->rrWhile == 0 || !port->reRoot)
            && !port->sync
            && !port->learn)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_DESIGNATED_LEARN);
        }

        if (((port->sync && !port->synced)
                || (port->reRoot && port->rrWhile != 0))
            && !port->operEdge
            && (port->learn || port->forward))
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_DESIGNATED_LISTEN);
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortRoleTransition_ChangeState: "
            "Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_PortRoleTransition_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_PortRoleTransition_ChangeState(
        node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_PortRoleTransition_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_PortRoleTransition_ChangeState(
            node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 17.24 Port State Transition state machine

#define STP_SM_PORT_STATE_TRANSITION_BEGIN (STP_SM_STATES + 2401)
#define STP_SM_PORT_STATE_TRANSITION_END \
            (3 + STP_SM_PORT_STATE_TRANSITION_BEGIN)

typedef enum
{
    STP_SM_DISCARDING = STP_SM_PORT_STATE_TRANSITION_BEGIN,
    STP_SM_LEARNING,
    STP_SM_FORWARDING
} StpStatePortStateTransitionType;


static
BOOL SwitchStpSm_PortStateTransition_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_PORT_STATE_TRANSITION_BEGIN
        && sm->state < STP_SM_PORT_STATE_TRANSITION_END;
}

static
void SwitchStpSm_PortStateTransition_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    switch (sm->state)
    {

    case STP_SM_DISCARDING:
        SwitchStp_DisableLearning(node, sw, port);
        port->learning = FALSE;
        SwitchStp_DisableForwarding(node, sw, port);
        port->forwarding = FALSE;

        break;

    case STP_SM_LEARNING:
        SwitchStp_EnableLearning(node, sw, port);
        port->learning = TRUE;

        break;

    case STP_SM_FORWARDING:
        port->tc = !(port->operEdge);
        SwitchStp_EnableForwarding(node, sw, port);
        port->forwarding = TRUE;

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortStateTransition_EnterState: "
            "Invalid state.\n");

        break;

    } //switch
}


static
BOOL SwitchStpSm_PortStateTransition_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_DISCARDING);
    }

    switch (sm->state)
    {

    case STP_SM_DISCARDING:
        if (port->learn)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_LEARNING);
        }

        break;

    case STP_SM_LEARNING:
        if (port->forward)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_FORWARDING);
        }

        if (!port->learn)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_DISCARDING);
        }

        break;

    case STP_SM_FORWARDING:
        if (!port->forward)
        {
             return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_DISCARDING);
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortStateTransition_ChangeState: "
            "Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_PortStateTransition_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_PortStateTransition_ChangeState(
        node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_PortStateTransition_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_PortStateTransition_ChangeState(
            node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 17.25 Topology Change state machine

#define STP_SM_TOPOLOGY_CHANGE_BEGIN (STP_SM_STATES + 2501)
#define STP_SM_TOPOLOGY_CHANGE_END (8 + STP_SM_TOPOLOGY_CHANGE_BEGIN)

typedef enum
{
    STP_SM_TC_INIT = STP_SM_TOPOLOGY_CHANGE_BEGIN,
    STP_SM_INACTIVE,
    STP_SM_DETECTED,
    STP_SM_NOTIFIED_TCN,
    STP_SM_NOTIFIED_TC,
    STP_SM_PROPAGATING,
    STP_SM_ACKNOWLEDGED,
    STP_SM_ACTIVE
} StpStateTopologyChangeType;


static
BOOL SwitchStpSm_TopologyChange_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_TOPOLOGY_CHANGE_BEGIN
        && sm->state < STP_SM_TOPOLOGY_CHANGE_END;
}


static
void SwitchStpSm_TopologyChange_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    switch (sm->state)
    {

    case STP_SM_TC_INIT:
        SwitchStp_FlushDatabase(node, sw, port);
        port->tcWhile = 0;
        port->tc = FALSE;
        port->tcAck = FALSE;
        port->tcProp = FALSE;

        break;

    case STP_SM_INACTIVE:
        port->rcvdTc = FALSE;
        port->rcvdTcn = FALSE;
        port->rcvdTcAck = FALSE;
        port->tc = FALSE;
        port->tcProp =  FALSE;

        break;

    case STP_SM_DETECTED:
        port->tcWhile = SwitchStp_NewTcWhile(node, sw, port);
        SwitchStp_SetTcPropBridge(node, sw, port);
        port->tc = FALSE;

        break;

    case STP_SM_NOTIFIED_TCN:
        port->tcWhile = SwitchStp_NewTcWhile(node, sw, port);

        break;

    case STP_SM_NOTIFIED_TC:
        port->rcvdTcn = FALSE;
        port->rcvdTc = FALSE;

        if (port->role == STP_ROLE_DESIGNATED)
        {
            port->tcAck = TRUE;
        }

        SwitchStp_SetTcPropBridge(node, sw, port);

        break;

    case STP_SM_PROPAGATING:
        port->tcWhile = SwitchStp_NewTcWhile(node, sw, port);
        SwitchStp_FlushDatabase(node, sw, port);
        port->tcProp = FALSE;

        break;

    case STP_SM_ACKNOWLEDGED:
        port->tcWhile = 0;
        port->rcvdTcAck = FALSE;

        break;

    case STP_SM_ACTIVE:

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_TopologyChange_EnterState: Invalid state.\n");

        break;

    } //switch
}


static
BOOL SwitchStpSm_TopologyChange_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_TC_INIT);
    }

    switch (sm->state)
    {

    case STP_SM_TC_INIT:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_INACTIVE);

        break;

    case STP_SM_INACTIVE:
        if (port->role == STP_ROLE_ROOT
            || port->role == STP_ROLE_DESIGNATED)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ACTIVE);
        }

        if (port->rcvdTc
            || port->rcvdTcn
            || port->rcvdTcAck
            || port->tc
            || port->tcProp)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_INACTIVE);
        }

        break;

    case STP_SM_DETECTED:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ACTIVE);

        break;

    case STP_SM_NOTIFIED_TCN:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_NOTIFIED_TC);

        break;

    case STP_SM_NOTIFIED_TC:
    case STP_SM_PROPAGATING:
    case STP_SM_ACKNOWLEDGED:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_ACTIVE);

        break;


    case STP_SM_ACTIVE:
        if (port->role != STP_ROLE_ROOT
            && port->role != STP_ROLE_DESIGNATED)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_TC_INIT);
        }

        if (port->tc)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_DETECTED);
        }

        if (port->rcvdTcn)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_NOTIFIED_TCN);
        }

        if (port->rcvdTc)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_NOTIFIED_TC);
        }

        if (port->tcProp
            && !port->operEdge)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_PROPAGATING);
        }

        if (port->rcvdTcAck)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_ACKNOWLEDGED);
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_TopologyChange_ChangeState: Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_TopologyChange_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_TopologyChange_ChangeState(node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_TopologyChange_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_TopologyChange_ChangeState(node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 17.26 Port Protocol Migration state machine

#define STP_SM_PORT_PROTOCOL_MIGRATION_BEGIN (STP_SM_STATES + 2601)
#define STP_SM_PORT_PROTOCOL_MIGRATION_END \
            (5 + STP_SM_PORT_PROTOCOL_MIGRATION_BEGIN)

typedef enum
{
    STP_SM_PPM_INIT = STP_SM_PORT_PROTOCOL_MIGRATION_BEGIN,
    STP_SM_SEND_RSTP,
    STP_SM_SENDING_RSTP,
    STP_SM_SEND_STP,
    STP_SM_SENDING_STP
} StpStatePortProtocolMigrationType;


static
BOOL SwitchStpSm_PortProtocolMigration_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_PORT_PROTOCOL_MIGRATION_BEGIN
        && sm->state < STP_SM_PORT_PROTOCOL_MIGRATION_END;
}


static
void SwitchStpSm_PortProtocolMigration_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    switch (sm->state)
    {

    case STP_SM_PPM_INIT:
        port->initPm = TRUE;
        port->mcheck = FALSE;

        break;

    case STP_SM_SEND_RSTP:
        port->mdelayWhile = sw->migrateTime;
        port->mcheck = FALSE;
        port->initPm= FALSE;
        port->sendRstp = TRUE;

        break;

    case STP_SM_SENDING_RSTP:
        port->rcvdRstp = FALSE;
        port->rcvdStp = FALSE;

        break;

    case STP_SM_SEND_STP:
        port->mdelayWhile = sw->migrateTime;
        port->sendRstp = FALSE;
        port->initPm = FALSE;

        break;

    case STP_SM_SENDING_STP:
        port->rcvdRstp = FALSE;
        port->rcvdStp = FALSE;

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortProtocolMigration_EnterState: "
            "Invalid state.\n");

        break;

    }
}


static
BOOL SwitchStpSm_PortProtocolMigration_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_PPM_INIT);
    }
    if (!port->portEnabled && !port->initPm)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_PPM_INIT);
    }

    switch (sm->state)
    {

    case STP_SM_PPM_INIT:
        if (sw->forceVersion < 2
            && port->portEnabled)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SEND_STP);
        }

        if (sw->forceVersion >= 2
            && port->portEnabled)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SEND_RSTP);
        }

        break;

    case STP_SM_SEND_RSTP:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SENDING_RSTP);

        break;

    case STP_SM_SENDING_RSTP:
        if (port->mdelayWhile != 0
            && (port->rcvdStp || port->rcvdRstp))
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_SENDING_RSTP);
        }

        if ((port->mdelayWhile == 0 && port->rcvdStp)
            || sw->forceVersion < 2)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SEND_STP);
        }

        if (port->mcheck)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SEND_RSTP);
        }

        break;

    case STP_SM_SEND_STP:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SENDING_STP);

        break;

    case STP_SM_SENDING_STP:
        if (port->mdelayWhile != 0
            && (port->rcvdStp || port->rcvdRstp))
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_SENDING_STP);
        }

        if (port->mdelayWhile == 0
            && port->rcvdRstp)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SEND_RSTP);
        }

        if (port->mcheck)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SEND_RSTP);
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortProtocolMigration_ChangeState: "
            "Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_PortProtocolMigration_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_PortProtocolMigration_ChangeState(
        node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_PortProtocolMigration_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_PortProtocolMigration_ChangeState(
            node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 17.27 Port Transmit state machine
#define STP_SM_PORT_TRANSMIT_BEGIN (STP_SM_STATES + 2701)
#define STP_SM_PORT_TRANSMIT_END (6 + STP_SM_PORT_TRANSMIT_BEGIN)

typedef enum
{
    STP_SM_TRANSMIT_INIT = STP_SM_PORT_TRANSMIT_BEGIN,
    STP_SM_TRANSMIT_PERIODIC,
    STP_SM_TRANSMIT_CONFIG,
    STP_SM_TRANSMIT_TCN,
    STP_SM_TRANSMIT_RSTP,
    STP_SM_TRANSMIT_IDLE
} StpStatePortTransmitType;


static
BOOL SwitchStpSm_PortTransmit_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_PORT_TRANSMIT_BEGIN
        && sm->state < STP_SM_PORT_TRANSMIT_END;
}


static
void SwitchStpSm_PortTransmit_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    switch (sm->state)
    {

    case STP_SM_TRANSMIT_INIT:
        port->newInfo = FALSE;
        port->helloWhen = 0;
        port->txCount = 0;

        break;

    case STP_SM_TRANSMIT_PERIODIC:
        port->newInfo = port->newInfo
            || (port->role == STP_ROLE_DESIGNATED
                || (port->role == STP_ROLE_ROOT
                    && port->tcWhile != 0));
        port->helloWhen = sw->rootTimes.helloTime;

        break;

    case STP_SM_TRANSMIT_CONFIG:
        port->newInfo = FALSE;
        SwitchStp_TransmitConfigBpdu(node, sw, port);
        port->txCount += 1;
        port->tcAck = FALSE;

        break;

    case STP_SM_TRANSMIT_TCN:
        port->newInfo = FALSE;
        SwitchStp_TransmitTcnBpdu(node, sw, port);
        port->txCount += 1;

        break;

    case STP_SM_TRANSMIT_RSTP:
        port->newInfo = FALSE;
        SwitchStp_TransmitRstBpdu(node, sw, port);
        port->txCount += 1;
        port->tcAck = FALSE;

        break;

    case STP_SM_TRANSMIT_IDLE:

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortTransmit_EnterState: Invalid state.\n");

        break;

    }
}


static
BOOL SwitchStpSm_PortTransmit_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_TRANSMIT_INIT);
    }

    switch (sm->state)
    {

    case STP_SM_TRANSMIT_INIT:
    case STP_SM_TRANSMIT_PERIODIC:
    case STP_SM_TRANSMIT_CONFIG:
    case STP_SM_TRANSMIT_TCN:
    case STP_SM_TRANSMIT_RSTP:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_TRANSMIT_IDLE);

        break;

    case STP_SM_TRANSMIT_IDLE:
        if (!(port->selected && !port->updtInfo))
        {
            break;
        }

        if (port->helloWhen == 0)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_TRANSMIT_PERIODIC);
        }

        if (!port->sendRstp
            && port->newInfo
            && port->txCount < sw->txHoldCount
            && port->role == STP_ROLE_DESIGNATED
            && port->helloWhen != 0)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_TRANSMIT_CONFIG);
        }

        if (!port->sendRstp
            && port->newInfo
            && port->txCount < sw->txHoldCount
            && port->role == STP_ROLE_ROOT
            && port->helloWhen != 0)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_TRANSMIT_TCN);
        }

        if (port->sendRstp
            && port->newInfo
            && port->txCount < sw->txHoldCount
            && port->helloWhen != 0
            && (port->role == STP_ROLE_ROOT
                || port->role == STP_ROLE_DESIGNATED))
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_TRANSMIT_RSTP);
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_PortTransmit_ChangeState: Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_PortTransmit_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_PortTransmit_ChangeState(node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_PortTransmit_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_PortTransmit_ChangeState(node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 17.29 Ageing Timer state machine

#define STP_SM_AGEING_TIMER_BEGIN (STP_SM_STATES + 2901)
#define STP_SM_AGEING_TIMER_END (2 + STP_SM_AGEING_TIMER_BEGIN)

typedef enum
{
    STP_SM_LONG_TIMER = STP_SM_AGEING_TIMER_BEGIN,
    STP_SM_SHORT_TIMER
} StpStateAgeingTimerType;


static
BOOL SwitchStpSm_AgeingTimer_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_AGEING_TIMER_BEGIN
        && sm->state < STP_SM_AGEING_TIMER_END;
}


static
void SwitchStpSm_AgeingTimer_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    switch (sm->state)
    {

    case STP_SM_LONG_TIMER:
        port->ageingTimer = sw->ageingTime;

        break;

    case STP_SM_SHORT_TIMER:
        port->ageingTimer = sw->rootTimes.forwardDelay;

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_AgeingTimer_EnterState: Invalid state.\n");

        break;

    } //switch
}


static
BOOL SwitchStpSm_AgeingTimer_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_SHORT_TIMER);
    }

    switch (sm->state)
    {

    case STP_SM_LONG_TIMER:
        if (sw->forceVersion == 0
            && port->tcWhile != 0)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_SHORT_TIMER);
        }

        break;

    case STP_SM_SHORT_TIMER:

        if (sw->forceVersion != 0
            || port->tcWhile == 0)
        {
            return SwitchStpSm_ChangeState(
                node, sw, sm, STP_SM_LONG_TIMER);
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_AgeingTimer_ChangeState: Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_AgeingTimer_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_AgeingTimer_ChangeState(node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_AgeingTimer_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_AgeingTimer_ChangeState(node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 17.30 operEdge Change Detection state machine

#define STP_SM_OPEREDGE_CHANGE_DEECTION_BEGIN (STP_SM_STATES + 3001)
#define STP_SM_OPEREDGE_CHANGE_DEECTION_END \
            (3 + STP_SM_OPEREDGE_CHANGE_DEECTION_BEGIN)

typedef enum
{
    STP_SM_GENERATE_TC = STP_SM_OPEREDGE_CHANGE_DEECTION_BEGIN,
    STP_SM_NOT_OPER_EDGE,
    STP_SM_OPER_EDGE
} StpStateOperEdgeChangeDetectionType;


static
BOOL SwitchStpSm_OperEdgeChangeDetection_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_OPEREDGE_CHANGE_DEECTION_BEGIN
        && sm->state < STP_SM_OPEREDGE_CHANGE_DEECTION_END;
}


static
void SwitchStpSm_OperEdgeChangeDetection_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    switch (sm->state)
    {

    case STP_SM_GENERATE_TC:
        port->tc = TRUE;

        break;

    case STP_SM_NOT_OPER_EDGE:

        break;

    case STP_SM_OPER_EDGE:

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_OperEdgeChangeDetection_EnterState: "
            "Invalid state.\n");

        break;

    } //switch
}


static
BOOL SwitchStpSm_OperEdgeChangeDetection_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_NOT_OPER_EDGE);
    }

    switch (sm->state)
    {

    case STP_SM_GENERATE_TC:
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_NOT_OPER_EDGE);

        break;

    case STP_SM_NOT_OPER_EDGE:

        if (port->operEdge)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_OPER_EDGE);
        }

        break;

    case STP_SM_OPER_EDGE:

        if (!port->operEdge)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_GENERATE_TC);
        }

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_OperEdgeChangeDetection_ChangeState: "
            "Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_OperEdgeChangeDetection_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat =
        SwitchStpSm_OperEdgeChangeDetection_ChangeState(node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_OperEdgeChangeDetection_EnterState(node, sw, port, sm);
        repeat =
            SwitchStpSm_OperEdgeChangeDetection_ChangeState(
                node, sw, port, sm);
    }

    return retVal;
}


// ..................................................................
// 18. Bridge Detection state machine

#define STP_SM_BRIDGE_DETECTION_BEGIN (STP_SM_STATES + 10001)
#define STP_SM_BRIDGE_DETECTION_END (2 + STP_SM_BRIDGE_DETECTION_BEGIN)

typedef enum
{
    STP_SM_BD_INIT = STP_SM_BRIDGE_DETECTION_BEGIN,
    STP_SM_BPDU_SEEN
} StpStateBridgeDetectionType;


static
BOOL SwitchStpSm_BridgeDetection_CheckStateIsInRange(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    return sm->state >= STP_SM_BRIDGE_DETECTION_BEGIN
        && sm->state < STP_SM_BRIDGE_DETECTION_END;
}


static
void SwitchStpSm_BridgeDetection_EnterState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    switch (sm->state)
    {

    case STP_SM_BD_INIT:
        port->operEdge = port->adminEdgePort;
        port->bpduReceived = FALSE;

        break;

    case STP_SM_BPDU_SEEN:
        port->operEdge = FALSE;

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_BridgeDetection_EnterState: Invalid state.\n");

        break;

    } //switch
}


static
BOOL SwitchStpSm_BridgeDetection_ChangeState(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{

    if (sm->state == STP_SM_STATES)
    {
        return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_BD_INIT);
    }

    switch (sm->state)
    {

    case STP_SM_BD_INIT:
        if (port->bpduReceived)
        {
            return SwitchStpSm_ChangeState(node, sw, sm, STP_SM_BPDU_SEEN);
        }

        break;

    case STP_SM_BPDU_SEEN:

        break;

    default:
        ERROR_ReportError(
            "SwitchStpSm_BridgeDetection_ChangeState: Invalid state.\n");

        break;

    } //switch

    return FALSE;
}


static
BOOL SwitchStpSm_BridgeDetection_Iterate(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm* sm)
{
    BOOL repeat = SwitchStpSm_BridgeDetection_ChangeState(
        node, sw, port, sm);
    BOOL retVal = repeat;

    while (repeat)
    {
        SwitchStpSm_BridgeDetection_EnterState(node, sw, port, sm);
        repeat = SwitchStpSm_BridgeDetection_ChangeState(
            node, sw, port, sm);
    }

    return retVal;
}


// ------------------------------------------------------------------
// General State machine routines


// NAME:        SwitchStpSm_IterateSwitchAndPorts
//
// PURPOSE:     Iterate over all switch and port state machines
//              till there are no more state changes.
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStpSm_IterateSwitchAndPorts(
    Node* node,
    MacSwitch* sw)
{
    SwitchPort* aPort;
    BOOL repeat;
    BOOL repeatPorts;
    SwitchStpSm* sm;

    ERROR_Assert(sw->runStp,
        "SwitchStpSm_IterateOverSwitchAndPortsAlternate1 : "
        "Attempting to iterate state machines when STP is off.\n");

    do
    {
        repeatPorts = FALSE;

        sm = sw->smPortRoleSelection;
        repeatPorts |= sm->sm_Iterate(node, sw, NULL, sm);

        aPort = sw->firstPort;
        while(aPort != NULL)
        {
            do
            {
                repeat = FALSE;

                sm = aPort->smPortTimers;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                sm = aPort->smPortInfo;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                sm = aPort->smPortRoleTransition;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                sm = aPort->smPortStateTransition;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                sm = aPort->smTopologyChange;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                sm = aPort->smPortProtocolMigration;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                sm = aPort->smPortTransmit;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                sm = aPort->smAgeingTimer;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                sm = aPort->smOperEdgeChangeDetection;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                sm = aPort->smBridgeDetection;
                repeat |= sm->sm_Iterate(node, sw, aPort, sm);

                repeatPorts |= repeat;

            } while (repeat);

            aPort = aPort->nextPort;
        }

    } while (repeatPorts);
}


// NAME:        SwitchStpSm_Create
//
// PURPOSE:     Create and initialize a state machine
//
// PARAMETERS:  function pointer to CheckStateIsInRange
//              function pointer to EnterState
//              function pointer to ChangeState
//              function pointer to Iterate
//
// RETURN:      None

static
SwitchStpSm* SwitchStpSm_Create(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    SwitchStpSm_CheckStateIsInRange sm_CheckStateIsInRange,
    SwitchStpSm_EnterState sm_EnterState,
    SwitchStpSm_TransitState sm_ChangeState,
    SwitchStpSm_Iterate sm_Iterate)
{
    SwitchStpSm* sm = (SwitchStpSm*) MEM_malloc(sizeof(SwitchStpSm));

    sm->state = STP_SM_STATES;
    sm->stateHasChanged = FALSE;

    sm->sm_CheckStateIsInRange = sm_CheckStateIsInRange;
    sm->sm_EnterState = sm_EnterState;
    sm->sm_ChangeState = sm_ChangeState;
    sm->sm_Iterate = sm_Iterate;

    return sm;
}


// NAME:        SwitchStp_PortSmInit
//
// PURPOSE:     Initialize the state machines for a port
//              Uses the STP_SM_CREATE macro
//
// PARAMETERS:
//
// RETURN:      None

#define STP_SM_CREATE(NODE, SW, PORT, SM_NAME)  \
    SwitchStpSm_Create(                         \
    NODE,                                       \
    SW,                                         \
    PORT,                                       \
    SwitchStpSm_##SM_NAME##_CheckStateIsInRange,\
    SwitchStpSm_##SM_NAME##_EnterState,         \
    SwitchStpSm_##SM_NAME##_ChangeState,        \
    SwitchStpSm_##SM_NAME##_Iterate)


static
void SwitchStp_PortSmInit(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    port->smPortTimers =
        STP_SM_CREATE(node, sw, port, PortTimers);
    port->smPortInfo =
        STP_SM_CREATE(node, sw, port, PortInfo);
    port->smPortRoleTransition =
        STP_SM_CREATE(node, sw, port, PortRoleTransition);
    port->smPortStateTransition =
        STP_SM_CREATE(node, sw, port, PortStateTransition);
    port->smTopologyChange =
        STP_SM_CREATE(node, sw, port, TopologyChange);
    port->smPortProtocolMigration =
        STP_SM_CREATE(node, sw, port, PortProtocolMigration);
    port->smPortTransmit =
        STP_SM_CREATE(node, sw, port, PortTransmit);
    port->smAgeingTimer =
        STP_SM_CREATE(node, sw, port, AgeingTimer);
    port->smOperEdgeChangeDetection =
        STP_SM_CREATE(node, sw, port, OperEdgeChangeDetection);
    port->smBridgeDetection =
        STP_SM_CREATE(node, sw, port, BridgeDetection);
}


// NAME:        SwitchStp_PortSmFree
//
// PURPOSE:     Deallocates the state machines for a port
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_PortSmFree(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port)
{
    MEM_free(port->smPortTimers);
    MEM_free(port->smPortInfo);
    MEM_free(port->smPortRoleTransition);
    MEM_free(port->smPortStateTransition);
    MEM_free(port->smTopologyChange);
    MEM_free(port->smPortProtocolMigration);
    MEM_free(port->smPortTransmit);
    MEM_free(port->smAgeingTimer);
    MEM_free(port->smOperEdgeChangeDetection);
    MEM_free(port->smBridgeDetection);
}


// NAME:        SwitchStp_ReceiveBpdu
//
// PURPOSE:     Receive a BPDU from the MAC protocol specific
//              layer. Validate the BPDU and extract the message
//              values.
//
// PARAMETERS:  received BPDU message
//
// RETURN:      None

void SwitchStp_ReceiveBpdu(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port,
    Message* msg)
{
    StpConfigBpdu* bpdu;

    if (!port->portEnabled)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    if (SwitchStp_IsValidBpdu(node, sw, port, msg))
    {
        SwitchStp_Trace(node, sw, port, msg, "Receive");

        port->bpduReceived = TRUE;

        port->msgType = SwitchStp_GetBpduType(msg);

        switch (port->msgType)
        {

        case STP_BPDUTYPE_CONFIG:
            port->rcvdBpdu = TRUE;
            port->stats.configBpdusReceived++;

            break;

        case STP_BPDUTYPE_RST:
            if (sw->forceVersion >= STP_VERSION_DEFAULT)
            {
                port->rcvdBpdu = TRUE;
            }
            port->stats.rstpBpdusReceived++;

            break;

        case STP_BPDUTYPE_TCN:
            port->rcvdBpdu = TRUE;
            port->stats.tcnsReceived++;

            break;

        default:
            ERROR_ReportError(
                "SwitchStp_ReceivePacket: Invalid received BPDU type.\n");

            break;

        } //switch

        if (port->msgType != STP_BPDUTYPE_TCN)
        {
            bpdu = (StpConfigBpdu*) MESSAGE_ReturnPacket(msg);

            SwitchStp_AssignPriority(
                bpdu->rootId,
                bpdu->rootCost,
                bpdu->swId,
                bpdu->portId,
                &port->msgPriority);
            SwitchStp_AssignTimes(
                SwitchStp_BpduTimeToClocktype(bpdu->messageAge),
                SwitchStp_BpduTimeToClocktype(bpdu->maxAge),
                SwitchStp_BpduTimeToClocktype(bpdu->helloTime),
                SwitchStp_BpduTimeToClocktype(bpdu->forwardDelay),
                &port->msgTimes);

            port->msgProtocolVersion = bpdu->bpduHdr.protocolVersion;
            port->msgFlags = bpdu->flags;

            if (sw->runStp)
            {
                // iterate SMs
                SwitchStpSm_IterateSwitchAndPorts(node, sw);
            }
        }
    }
    else
    {
        SwitchStp_Trace(node, sw, port, NULL, "Invalid BPDU received");
    }

    MESSAGE_Free(node, msg);
}


// NAME:        SwitchStp_SetVarsWhenStpIsOff
//
// PURPOSE:     Initialize variables in case the user specifies
//              that STP should not be run on this switch.
//
// PARAMETERS:
//
// RETURN:      None
//

void SwitchStp_SetVarsWhenStpIsOff(
    Node *node,
    MacSwitch* sw)
{
    SwitchPort* aPort = sw->firstPort;

    while (aPort != NULL)
    {
        if (aPort->macEnabled)
        {
            aPort->learning = TRUE;
            aPort->forwarding = TRUE;
            aPort->role = STP_ROLE_DESIGNATED;

            SwitchPort_EnableForwarding(node, aPort);
        }
        else
        {
            aPort->learning = FALSE;
            aPort->forwarding = FALSE;
            aPort->role = STP_ROLE_DISABLED;

            SwitchPort_DisableForwarding(node, aPort);
        }

        aPort = aPort->nextPort;
    }
}


// NAME:        SwitchStp_PortVarInit
//
// PURPOSE:     Initialize port variable that are not handled
//              by the state machines.
//
// PARAMETERS:
//
// RETURN:      None
//

static
void SwitchStp_PortVarInit(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port)
{
    SwitchStp_AssignPortId(port->priority, port->portNumber, &port->portId);

    port->macEnabled = TRUE;
    port->macOperational = TRUE;
    port->portEnabled = TRUE;

    // 14.8.2.4
    if (sw->forceVersion >= STP_VERSION_DEFAULT)
    {
        port->mcheck = TRUE;
    }
    else
    {
        port->mcheck = FALSE;
    }

}


// NAME:        SwitchStp_PortPathCostInit
//
// PURPOSE:     Initialize path cost for a port.
//              In the absence of input, the inverse relation
//              derived from Table 17.7 is used.
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_PortPathCostInit(
    Node* node,
    MacSwitch* sw,
    SwitchPort* port,
    const NodeInput* nodeInput,
    int portNumber)
{
    int intInput;
    BOOL retVal;
    char errString[MAX_STRING_LENGTH];
    double interfaceBw;
    //Address address;

   // NetworkGetInterfaceInfo(
   //     node, port->macData->interfaceIndex, &address);

    // Read path cost for the port
    // Format is [switch ID] SWITCH-PORT-PATH-COST[port ID]   <value>
    //        or [port addr] SWITCH-PORT-PATH-COST            <value>
    //
    IO_ReadIntInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-PATH-COST",
        portNumber,
        TRUE,
        &retVal,
        &intInput);

    if (!retVal)
    {
        char buf[MAX_STRING_LENGTH];

        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-PORT-PATH-COST",
            &retVal,
            buf);

        if (retVal)
        {
            intInput = atoi(buf);
        }
    }

    if (retVal)
    {
        if (intInput >= STP_PATH_COST_MIN
            && intInput <= STP_PATH_COST_MAX)
        {
            port->pathCost = intInput;
        }
        else
        {
            sprintf(errString,
                "SwitchStp_PortPathCostInit: "
                "Error in SWITCH-PORT-PATH-COST in user configuration "
                "for port %d in switch %d\n"
                "Cost of the path should be between %d to %d",
                portNumber, node->nodeId,
                STP_PATH_COST_MIN, STP_PATH_COST_MAX);

            ERROR_ReportError(errString);
        }
    }
    else
    {
        int factor;

        // Assign default value based on interface bandwidth.
        interfaceBw = (double)port->macData->bandwidth;
        ERROR_Assert(interfaceBw > 0,
            "SwitchStp_PortPathCostInit: Port bandwidth not positive.\n");

        factor = MAX((int)log10(interfaceBw / 10000), 0);
        port->pathCost = (unsigned)
            (STP_PATH_COST_MAX_DEFAULT / pow(10.0, factor));
        if (interfaceBw > STP_BANDWIDTH_MAX)
        {
            port->pathCost = 1;
        }
    }
}


// NAME:        SwitchStp_PortInit
//
// PURPOSE:     Read spanning tree specific port variables
//              from user input. These are:
//                  port priority
//                  point to point port
//                  edge port
//              Calls functions to initialize
//                  path cost
//                  other port variables
//                  state machines
//
// PARAMETERS:
//
// RETURN:      None

void SwitchStp_PortInit(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port,
    const NodeInput *nodeInput,
    int portNumber)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];
    int intInput;
    char errString[MAX_STRING_LENGTH];

    // Read Port priority
    // Default is as per Table 17.6
    // Format is [switch ID] SWITCH-PORT-PRIORITY[port ID]   <value>
    //        or [port addr] SWITCH-PORT-PRIORITY            <value>
    //
    IO_ReadIntInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-PRIORITY",
        portNumber,
        TRUE,
        &retVal,
        &intInput);

    if (!retVal)
    {
        char buf[MAX_STRING_LENGTH];

        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-PORT-PRIORITY",
            &retVal,
            buf);

            if (retVal)
            {
                intInput = atoi(buf);
            }
    }

    if (retVal)
    {
        if (intInput >= STP_PORT_PRIORITY_MIN
            && intInput <= STP_PORT_PRIORITY_MAX)
        {
            // Round off to nearest multiple
            port->priority = (unsigned char)
                (((intInput + STP_PORT_PRIORITY_MULTIPLE / 2)
                    / STP_PORT_PRIORITY_MULTIPLE)
                * STP_PORT_PRIORITY_MULTIPLE);
        }
        else
        {
            sprintf(errString,
                "MacStpPort_Init: "
                "Error in SWITCH-PORT-PRIORITY in user configuration "
                "for port %d in switch %d\n"
                "Priority should be between %d to %d\n",
                portNumber,
                node->nodeId,
                STP_PORT_PRIORITY_MIN,
                STP_PORT_PRIORITY_MAX);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        port->priority = STP_PORT_PRIORITY_DEFAULT;
    }

    // Read whether port services a point-to-point link
    // Format is [switch ID] SWITCH-PORT-POINT-TO-POINT[port ID]   <value>
    //        or [port addr] SWITCH-PORT-POINT-TO-POINT            <value>
    // where value is one of AUTO | FORCE-TRUE | FORCE-FALSE
    // Default is AUTO as per 6.4.3
    //
    IO_ReadStringInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-POINT-TO-POINT",
        portNumber,
        TRUE,                       // ?
        &retVal,
        buf);

    if (!retVal)
    {
        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-PORT-POINT-TO-POINT",
            &retVal,
            buf);
    }

    if (retVal)
    {
        if (!strcmp(buf, "AUTO"))
        {
            port->adminPointToPointMac = SWITCH_ADMIN_P2P_AUTO;
        }
        else if (!strcmp(buf, "FORCE-TRUE"))
        {
            port->adminPointToPointMac = SWITCH_ADMIN_P2P_FORCE_TRUE;
        }
        else if (!strcmp(buf, "FORCE-FALSE"))
        {
            port->adminPointToPointMac = SWITCH_ADMIN_P2P_FORCE_FALSE;
        }
        else
        {
            sprintf(errString,
                "MacStpPort_Init: Error in "
                "SWITCH-PORT-POINT-TO-POINT in user configuration "
                "for port %d in switch %d\n"
                "Options are AUTO or FORCE-TRUE or FORCE-FALSE\n",
                portNumber,
                node->nodeId);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        port->adminPointToPointMac = SWITCH_ADMIN_P2P_AUTO;
    }

    switch (port->adminPointToPointMac)
    {
    case SWITCH_ADMIN_P2P_AUTO:
        if (port->macData->macProtocol == MAC_PROTOCOL_LINK)
        {
            port->operPointToPointMac = TRUE;
        }
        else
        {
            port->operPointToPointMac = FALSE;
        }
        break;
    case SWITCH_ADMIN_P2P_FORCE_TRUE:
        port->operPointToPointMac = TRUE;
        break;
    case SWITCH_ADMIN_P2P_FORCE_FALSE:
        port->operPointToPointMac = FALSE;
        break;
    default:
        ERROR_ReportError("SwitchStp_PortInit: Invalid adminP2P value.\n");
        break;
    } //switch


    // Read whether port is the only port on a LAN
    // Format is [switch ID] SWITCH-PORT-EDGE[port ID]   NO | YES
    //        or [port addr] SWITCH-PORT-EDGE            NO | YES
    // Default is NO as per 14.8.2.3.2
    //
    IO_ReadStringInstance(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PORT-EDGE",
        portNumber,
        TRUE,
        &retVal,
        buf);

    if (!retVal)
    {
        IO_ReadStringUsingIpAddress(
            node,
            port->macData->interfaceIndex,
            nodeInput,
            "SWITCH-PORT-EDGE",
            &retVal,
            buf);
    }

    if (retVal)
    {
        if (!strcmp(buf, "YES"))
        {
            port->adminEdgePort = TRUE;
        }
        else if (!strcmp(buf, "NO"))
        {
            port->adminEdgePort = FALSE;
        }
        else
        {
            sprintf(errString,
                "MacStpPort_Init: "
                "Error in SWITCH-PORT-EDGE in "
                "user configuration for port %d in switch %d\n"
                "Options are YES or NO",
                portNumber,
                node->nodeId);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        port->adminEdgePort = FALSE;
    }

    SwitchStp_PortPathCostInit(node, sw, port, nodeInput, portNumber);

    SwitchStp_PortVarInit(node, sw, port);

    SwitchStp_PortSmInit(node, sw, port);

}


// NAME:        SwitchStp_Init
//
// PURPOSE:     Reads switch specific spanning tree user input.
//                  switch priority
//                  hello time
//                  max age
//                  forward delay
//                  database ageing time
//                  hold count
//              Also initializes the switch state machine and
//              initializes variables.
//
// PARAMETERS:
//
// RETURN:      None

void SwitchStp_Init(
    Node* node,
    MacSwitch* sw,
    const NodeInput* nodeInput)
{
    BOOL retVal;
    int intInput;
    clocktype timeInput;
    char buf[MAX_STRING_LENGTH];
    char errString[MAX_STRING_LENGTH];

    // Read switch priority
    // Format is SWITCH-PRIORITY <value>
    // Default is as per Table 17.6
    //
    IO_ReadInt(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-PRIORITY",
        &retVal,
        &intInput);

    if (retVal)
    {
        if (intInput >= STP_SWITCH_PRIORITY_MIN
            && intInput <= STP_SWITCH_PRIORITY_MAX)
        {
            // Round of to nearest permitted multiple
            sw->priority = (unsigned short)
                (((intInput + STP_SWITCH_PRIORITY_MULTIPLE / 2)
                     / STP_SWITCH_PRIORITY_MULTIPLE)
                * STP_SWITCH_PRIORITY_MULTIPLE);
        }
        else
        {
            sprintf(errString,
                "SwitchStp_Init: "
                "Error in SWITCH-PRIORITY in user configuration.\n"
                "Switch %d has a priority that is out of range\n"
                "Priority should be between %d to %d\n",
                node->nodeId,
                STP_SWITCH_PRIORITY_MIN, STP_SWITCH_PRIORITY_MAX);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->priority = STP_SWITCH_PRIORITY_DEFAULT;
    }

    // Read hello time
    // Format is SWITCH-HELLO-TIME <value>
    // Default is as per Table 17.5
    //
    IO_ReadTime(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-HELLO-TIME",
        &retVal,
        &timeInput);

    if (retVal)
    {
        if (timeInput >= STP_HELLO_TIME_MIN &&
            timeInput <= STP_HELLO_TIME_MAX)
        {
            sw->swTimes.helloTime = timeInput;
        }
        else
        {
            char minTime[MAX_STRING_LENGTH];
            char maxTime[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(STP_HELLO_TIME_MIN, minTime);
            TIME_PrintClockInSecond(STP_HELLO_TIME_MAX, maxTime);

            sprintf(errString,
                "SwitchStp_Init: "
                "Error in SWITCH-HELLO-TIME in user configuration.\n"
                "Hello time should be between %s to %s seconds.\n",
                minTime, maxTime);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->swTimes.helloTime = STP_HELLO_TIME_DEFAULT;
    }

    // Read max age
    // Format is SWITCH-MAX-AGE <value>
    // Default is as per Table 17.5
    //
    IO_ReadTime(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-MAX-AGE",
        &retVal,
        &timeInput);

    if (retVal)
    {
        if (timeInput >= STP_MAX_AGE_MIN
            && timeInput <= STP_MAX_AGE_MAX)
        {
            sw->swTimes.maxAge = timeInput;
        }
        else
        {
            char minTime[MAX_STRING_LENGTH];
            char maxTime[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(STP_MAX_AGE_MIN, minTime);
            TIME_PrintClockInSecond(STP_MAX_AGE_MAX, maxTime);

            sprintf(errString,
                "SwitchStp_Init: "
                "Error in SWITCH-MAX-AGE in user configuration.\n"
                "Max age should be between %s to %s seconds.\n",
                minTime, maxTime);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->swTimes.maxAge = STP_MAX_AGE_DEFAULT;
    }


    // Read forward delay
    // Format is SWITCH-FORWARD-DELAY <value>
    // Default is as per Table 17.5
    //
    IO_ReadTime(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-FORWARD-DELAY",
        &retVal,
        &timeInput);

    if (retVal)
    {
        if (timeInput >= STP_FORWARD_DELAY_MIN &&
            timeInput <= STP_FORWARD_DELAY_MAX)
        {
            sw->swTimes.forwardDelay = timeInput;
        }
        else
        {
            char minTime[MAX_STRING_LENGTH];
            char maxTime[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(STP_FORWARD_DELAY_MIN, minTime);
            TIME_PrintClockInSecond(STP_FORWARD_DELAY_MAX, maxTime);

            sprintf(errString,
                "SwitchStp_Init: "
                "Error in SWITCH-FORWARD-DELAY in user configuration.\n"
                "Forward delay should be between %s to %s seconds.\n",
                 minTime, maxTime);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->swTimes.forwardDelay = STP_FORWARD_DELAY_DEFAULT;
    }

    // Validate times as per 8.10.2
    ERROR_Assert(2 * (sw->swTimes.forwardDelay - SECOND)
        >= sw->swTimes.maxAge,
        "Switch_StpInit: Constraint not satisfied.\n"
        "2 * (Switch_Forward_Delay - 1.0 seconds) >= Switch_Max_Age\n");

    ERROR_Assert(2 * sw->swTimes.maxAge
        >= 2 * (sw->swTimes.helloTime + SECOND),
        "SwitchStp_Init: Constraint not satisfied.\n"
        "Switch_Max_Age >= 2 * (Switch_Hello_Time + 1.0 seconds)\n");


    // Read long ageing time
    // Format is SWITCH-DATABASE-AGEING-TIME <value>
    // Default is as per Table 7.4
    //
    IO_ReadTime(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-DATABASE-AGEING-TIME",
        &retVal,
        &timeInput);

    if (!retVal)
    {
        IO_ReadTime(
            node->nodeId,
            ANY_ADDRESS,
            nodeInput,
            "SWITCH-DATABASE-AGING-TIME",
            &retVal,
            &timeInput);
    }

    if (retVal)
    {
        if (timeInput >= STP_AGEING_TIME_MIN
            && timeInput <= STP_AGEING_TIME_MAX)
        {
            sw->ageingTime = timeInput;
        }
        else
        {
            char minAge[MAX_STRING_LENGTH];
            char maxAge[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(STP_AGEING_TIME_MIN, minAge);
            TIME_PrintClockInSecond(STP_AGEING_TIME_MAX, maxAge);

            sprintf(errString,
                "SwitchStp_Init: "
                "Error in SWITCH-DATABASE-AGEING-TIME in "
                "user configuration for switch %d\n"
                "Filter database ageing time should be "
                "between %s to %s seconds\n",
                node->nodeId, minAge, maxAge);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->ageingTime = STP_AGEING_TIME_DEFAULT;
    }


    // Read transmission hold count
    // Format is SWITCH-HOLD-COUNT <value>
    // Default is as per Table 17.5
    //
    IO_ReadInt(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-HOLD-COUNT",
        &retVal,
        &intInput);

    if (retVal)
    {
        if (intInput >= STP_TX_HOLD_COUNT_MIN
            && intInput <= STP_TX_HOLD_COUNT_MAX)
        {
            sw->txHoldCount = intInput;
        }
        else
        {
            sprintf(errString,
                "SwitchStp_Init: "
                "Error in SWITCH-HOLD-COUNT in user configuration.\n"
                "Hold count should be between %d to %d\n",
                STP_TX_HOLD_COUNT_MIN, STP_TX_HOLD_COUNT_MAX);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->txHoldCount = STP_TX_HOLD_COUNT_DEFAULT;
    }

    // Read whether STP should be run
    // Format is SWITCH-RUN-STP YES | NO
    // Default is YES
    //
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "SWITCH-RUN-STP",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "NO") == 0)
        {
            sw->runStp = FALSE;
        }
        else if (strcmp(buf, "YES") == 0)
        {
            sw->runStp = TRUE;
        }
        else
        {
            sprintf(errString,
                "MacStp_Init: "
                "Error in SWITCH-RUN-STP in user configuration "
                "for switch %d\n"
                "Options are YES or NO\n",
                node->nodeId);
            ERROR_ReportError(errString);
        }
    }
    else
    {
        sw->runStp = TRUE;
    }

    sw->swTimes.messageAge = 0;

    sw->smPortRoleSelection =
        STP_SM_CREATE(node, sw, NULL, PortRoleSelection);

    sw->forceVersion = STP_VERSION_DEFAULT;
    SwitchStp_AssignSwId(sw->priority, sw->swAddr, &sw->swId);
    SwitchStp_AssignPortId(0, 0, &sw->rootPortId);
    SwitchStp_AssignPriority(
        sw->swId, 0, sw->swId, sw->rootPortId, &sw->swPriority);
    sw->rootPriority = sw->swPriority;
    sw->rootTimes = sw->swTimes;
    sw->migrateTime = STP_MIGRATE_TIME;
}


// ------------------------------------------------------------------
// STP interface failure functions


// NAME:        SwitchStp_EnableInterface
//
// PURPOSE:     Resets variables and iterates the state machines
//              to determine port roles when an interface is
//              enabled.
//
// PARAMETERS:
//
// RETURN:      None

void SwitchStp_EnableInterface(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port)
{
    if (port->portEnabled)
    {
        return;
    }

    SwitchStp_Trace(node, sw, port, NULL, "Interface enabled.");

    SwitchStp_PortVarInit(node, sw, port);
    port->macEnabled = TRUE;
    port->macOperational = TRUE;
    port->portEnabled = TRUE;
    port->tcProp = TRUE;

    port->role = STP_ROLE_DESIGNATED;
    port->selectedRole = STP_ROLE_DESIGNATED;

    port->reselect = TRUE;
    port->selected = FALSE;

    SwitchStpSm_ChangeState(node, sw, port->smPortTimers, STP_SM_STATES);
    SwitchStpSm_ChangeState(node, sw, port->smPortInfo, STP_SM_STATES);
    SwitchStpSm_ChangeState(
        node, sw, port->smPortRoleTransition, STP_SM_STATES);
    SwitchStpSm_ChangeState(
        node, sw, port->smPortStateTransition, STP_SM_STATES);
    SwitchStpSm_ChangeState(
        node, sw, port->smTopologyChange, STP_SM_STATES);
    SwitchStpSm_ChangeState(node, sw,
        port->smPortProtocolMigration, STP_SM_STATES);
    SwitchStpSm_ChangeState(
        node, sw, port->smPortTransmit, STP_SM_STATES);
    SwitchStpSm_ChangeState(node, sw, port->smAgeingTimer, STP_SM_STATES);
    SwitchStpSm_ChangeState(node, sw,
        port->smOperEdgeChangeDetection, STP_SM_STATES);
    SwitchStpSm_ChangeState(
        node, sw, port->smBridgeDetection, STP_SM_STATES);

    if (sw->runStp)
    {
        // iterate SMs
        SwitchStpSm_IterateSwitchAndPorts(node, sw);
    }
    else
    {
        SwitchStp_SetVarsWhenStpIsOff(node, sw);
    }
}


// NAME:        SwitchStp_DisableInterface
//
// PURPOSE:     Trigger the functions that will generate the
//              appropriate topology change indications at
//              the other ports of the switch when a port is
//              disabled.
//
// PARAMETERS:
//
// RETURN:      None

void SwitchStp_DisableInterface(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port)
{
    if (!port->portEnabled)
    {
        return;
    }

    SwitchStp_Trace(node, sw, port, NULL, "Interface disabled.");

    port->macEnabled = FALSE;
    port->macOperational = FALSE;
    port->portEnabled = FALSE;

    port->reselect = TRUE;
    port->selected = FALSE;

    if (sw->runStp)
    {
        // iterate SMs
        SwitchStpSm_IterateSwitchAndPorts(node, sw);
    }
    else
    {
        SwitchStp_SetVarsWhenStpIsOff(node, sw);
    }

}


// ------------------------------------------------------------------
// STP stats and finalize functions


// NAME:        SwitchStp_PortPrintStats
//
// PURPOSE:     Print port related spanning tree stats.
//
//              Currently, only RST BPDUs are considered.
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_PortPrintStats(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port)
{
    char buf[MAX_STRING_LENGTH];


    char portAddr[MAX_STRING_LENGTH];

     NetworkIpGetInterfaceAddressString(
        node, port->macData->interfaceIndex, portAddr);

    sprintf(buf,
        "RST BPDUs sent = %u", port->stats.rstpBpdusSent);
    IO_PrintStat(node, "Mac-Switch", "Port",
        portAddr,
        -1, //port->portNumber,
         buf);

    sprintf(buf,
        "RST BPDUs received = %u", port->stats.rstpBpdusReceived);
    IO_PrintStat(node, "Mac-Switch", "Port",
        portAddr,
        -1, //port->portNumber,
        buf);

    if (port->stats.configBpdusSent != 0
        || port->stats.configBpdusReceived != 0)
    {
        sprintf(buf,
            "Config BPDUs sent = %u", port->stats.configBpdusSent);
        IO_PrintStat(node, "Mac-Switch", "Port",
            portAddr,
            -1, //port->portNumber,
            buf);

        sprintf(buf,
            "Config BPDUs received = %u", port->stats.configBpdusReceived);
        IO_PrintStat(node, "Mac-Switch", "Port",
            portAddr,
            -1, //port->portNumber,
            buf);
    }

    if (port->stats.tcnsSent != 0 || port->stats.tcnsReceived != 0)
    {
        sprintf(buf,
            "Config BPDUs sent = %u", port->stats.tcnsSent);
        IO_PrintStat(node, "Mac-Switch", "Port",
            portAddr,
            -1, //port->portNumber,
            buf);

        sprintf(buf,
            "Config BPDUs received = %u", port->stats.tcnsReceived);
        IO_PrintStat(node, "Mac-Switch", "Port",
            portAddr,
            -1, //port->portNumber,
             buf);
    }
}


// NAME:        SwitchStp_PortFinalize
//
// PURPOSE:     Print port stats and free STP related memory.
//
// PARAMETERS:
//
// RETURN:      None

void SwitchStp_PortFinalize(
    Node *node,
    MacSwitch* sw,
    SwitchPort* port)
{
    if (node->macData[port->macData->interfaceIndex]->macStats
        && port->isPortStatEnabled)
    {
        SwitchStp_PortPrintStats(node, sw, port);
    }

    SwitchStp_PortSmFree(node, sw, port);
}


// NAME:        SwitchStp_PrintStats
//
// PURPOSE:     Print switch related STP stats.
//
// PARAMETERS:
//
// RETURN:      None

static
void SwitchStp_PrintStats(
    Node *node,
    MacSwitch* sw)
{
    // None at present
}


// NAME:        SwitchStp_Finalize
//
// PURPOSE:     Print switch related stats and free switch SM.
//
// PARAMETERS:
//
// RETURN:      None

void SwitchStp_Finalize(
    Node *node,
    MacSwitch* sw)
{
    if (node->macData[0]->macStats
        && sw->firstPort->isPortStatEnabled)
    {
        SwitchStp_PrintStats(node, sw);
    }

    MEM_free(sw->smPortRoleSelection);
}


// ------------------------------------------------------------------
// STP message / event functions


// NAME:        SwitchStp_TickTimerEvent
//
// PURPOSE:     Execute SMs and age out database entries at
//              every tick. The tick has a granularity of 1 S.
//
// PARAMETERS:
//
// RETURN:      None

void SwitchStp_TickTimerEvent(
    Node *node,
    MacSwitch* sw)
{
    SwitchPort* aPort = sw->firstPort;
    clocktype currentTime = node->getNodeTime();

    ERROR_Assert(sw->runStp,
        "SwitchStp_TickTimerEvent : "
        "Received Tick event when STP is off.\n");

    aPort = sw->firstPort;
    while (aPort != NULL)
    {
        aPort->tick = TRUE;
        aPort = aPort->nextPort;
    }

    SwitchStpSm_IterateSwitchAndPorts(node, sw);

    SwitchStp_Trace(node, sw, NULL, NULL, "PortStates");
    SwitchStp_Trace(node, sw, NULL, NULL, "Vars");

    aPort = sw->firstPort;
    while (aPort != NULL)
    {
        // Delete entries learnt on port
        // that are ageingTimer older than currentTime
        SwitchDb_EntryAgeOutByPort(
            sw->db, aPort->portNumber, SWITCH_DB_DYNAMIC,
            currentTime, aPort->ageingTimer);

        aPort = aPort->nextPort;
    }
}



// ------------------------------------------------------------------

