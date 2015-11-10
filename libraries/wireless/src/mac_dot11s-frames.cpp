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
#include "mac_dot11.h"
#include "mac_dot11-mgmt.h"
#include "mac_dot11s.h"
#include "mac_dot11s-frames.h"


// ------------------------------------------------------------------

// Predefined field values for path protocol.
static
unsigned int DOT11s_PathProtocolId[] =
    {
        0x000FAC00,                     // HWMP
        0x000FAC01,                     // OLSR
        0x000FACFF                      // Null protocol
    };


// Predefined field values for path metric.
static
unsigned int DOT11s_PathMetricId[] =
    {
        0x000FAC00,                     // Airtime path metric
        0x000FACFF                      // Null metric
    };


/**
FUNCTION   :: Dot11s_GetPathProtocolFromId
LAYER      :: MAC
PURPOSE    :: Get path protocol enum given path protocol field ID
PARAMETERS ::
+ pathProtocolId : unsigned int : ID value used in IE fields
RETURN     :: DOT11s_PathProtocol enum value
**/

static
DOT11s_PathProtocol Dot11s_GetPathProtocolFromId(
    unsigned int pathProtocolId)
{
    for (int i = 0; i <= DOT11s_PATH_PROTOCOL_NULL; i++)
    {
        if (DOT11s_PathProtocolId[i] == pathProtocolId)
        {
            return DOT11s_PathProtocol(i);
        }
    }

    ERROR_ReportError(
        "Dot11s_GetPathProtocol: "
        "No match between protocol and ID");

    return DOT11s_PathProtocol(0); //keep compiler happy
}


/**
FUNCTION   :: Dot11s_GetPathMetricFromId
LAYER      :: MAC
PURPOSE    :: Get path metric enum given path metric field ID
PARAMETERS ::
+ pathMetricId : unsigned int : ID value used in IEs
RETURN     :: DOT11s_PathMetric enum value
**/

static
DOT11s_PathMetric Dot11s_GetPathMetricFromId(
    unsigned int pathMetricId)
{
    for (int i = 0; i <= DOT11s_PATH_METRIC_NULL; i++)
    {
        if (DOT11s_PathMetricId[i] == pathMetricId)
        {
            return DOT11s_PathMetric(i);
        }
    }

    ERROR_ReportError(
        "Dot11s_GetPathMetric: "
        "No match between metric and ID");

    return DOT11s_PathMetric(0);   //keep compiler happy
}


/**
FUNCTION   :: Dot11s_StoreIeLength
LAYER      :: MAC
PURPOSE    :: Place length value of IE at appropriate place in data stream.
PARAMETERS ::
+ element   : DOT11_Ie*     : IE pointer
RETURN     :: void
**/

static
void Dot11s_StoreIeLength(
    DOT11_Ie* element)
{
    element->data[1] =
        (unsigned char)(element->length - DOT11_IE_HDR_LENGTH);
}


/**
FUNCTION   :: Dot11s_AppendToElement
LAYER      :: MAC
PURPOSE    :: Append a field to an IE.
PARAMETERS ::
+ element   : DOT11_Ie*     : IE pointer
+ field     : Type T        : field to append
+ length    : int           : length of field, optional
RETURN     :: void
NOTES      :: Does not work when appending strings.
                The length parameter, if omitted, defaults to
                the size of the field parameter.
                It is good practice to pass the length value to
                avoid surprises due to sizeof(field) across platforms.
**/

template <class T>
static
void Dot11s_AppendToElement(
    DOT11_Ie* element,
    T field,
    int length = 0)
{
    int fieldLength = (length == 0) ? (int)sizeof(field) : length;

    ERROR_Assert(sizeof(field) == fieldLength,
        "Dot11s_AppendToElement: "
        "size of field does not match length parameter.\n");
    ERROR_Assert(element->length + fieldLength <= DOT11_IE_LENGTH_MAX,
        "Dot11s_AppendToElement: "
        "appending would exceed permissible IE length.\n");

    memcpy(element->data + element->length, &field, (size_t)fieldLength);
    element->length += fieldLength;
}


/**
FUNCTION   :: Dot11s_StoreInElement
LAYER      :: MAC
PURPOSE    :: Utility function to store a field in an IE at given position.
PARAMETERS ::
+ element   : DOT11_Ie*     : pointer to IE
+ storeAt   : int           : position at which to store field
+ field     : Type T        : field that is to be stored
+ length    : int           : length of field, optional
RETURN     :: void
NOTES      :: Not currently used.
                The length parameter, if omitted, defaults to
                the size of the field parameter.
**/

template <class T>
static
void Dot11s_StoreInElement(
    DOT11_Ie* element,
    int storeAt,
    T field,
    int length = 0)
{
    int fieldLength = (length == 0) ? sizeof(field) : length;

    ERROR_Assert(sizeof(field) == fieldLength,
        "Dot11s_StoreInElement: "
        "size of field does not match length parameter.\n");
    ERROR_Assert(storeAt + fieldLength <= element->length,
        "Dot11s_StoreInElement: "
        "storing field length would exceed permissible IE length.\n");

    memcpy(element->data + storeAt, &field, fieldLength);
}


/**
FUNCTION   :: Dot11s_ReadFromElement
LAYER      :: MAC
PURPOSE    :: Utility function to read a field from an IE.
PARAMETERS ::
+ element   : DOT11_Ie*     : pointer to IE
+ readOffset : int*         : position from which to read
+ field     : Type T        : field to be read
+ length    : int           : length to be read, optional
RETURN     :: void
NOTES      :: Does not work when reading strings.
                It is good practice to pass length to avoid surprises
                due to sizeof(field) across platforms.
**/

template <class T>
static
void Dot11s_ReadFromElement(
    DOT11_Ie* element,
    int* readOffset,
    T* field,
    int length = 0)
{
    int fieldLength = (length == 0) ? sizeof(*field) : length;

    ERROR_Assert(sizeof(*field) == fieldLength,
        "Dot11s_ReadFromElement: "
        "size of field does not match length parameter.\n");
    ERROR_Assert(*readOffset + fieldLength <= element->length,
        "Dot11s_ReadFromElement: "
        "read length goes beyond IE length.\n");

    memcpy(field, element->data + *readOffset, (size_t)fieldLength);
    *readOffset += fieldLength;
}


/**
FUNCTION   :: Dot11s_ReadHdrFromElement
LAYER      :: MAC
PURPOSE    :: Parses and reads the type/ID and length value from an IE.
PARAMETERS ::
+ element   : DOT11_Ie*     : IE to be parsed
+ hdr       : DOT11_IeHdr*  : header that will contain the read type/length
RETURN     :: void
**/

static
void Dot11s_ReadHdrFromElement(
    DOT11_Ie* element,
    DOT11_IeHdr* hdr)
{
    hdr->id = *(element->data);
    hdr->length = *(element->data + 1);
}


/**
FUNCTION   :: Dot11s_AssignToElement
LAYER      :: MAC
PURPOSE    :: Utility fn to assign to an IE from a given byte stream.
PARAMETERS ::
+ element   : DOT11_Ie*     : IE pointer
+ source    : unsigned char* : source stream
+ assignFrom: int*          : source offset to assign from
+ assignLimit: int          : limit of source offset, use 0 if not known
RETURN     :: void
NOTES      :: The assignment position is advanced beyond the read IE
                ready for the next call to this function.
                If the assign limit is non-zero, checks if it may be
                exceeded during assignment. Typically, the assign limit
                would be the size of the received frame.
**/

void Dot11s_AssignToElement(
    DOT11_Ie* element,
    unsigned char* source,
    int* assignFrom,
    int assignLimit)
{
    DOT11_IeHdr ieHdr;

    element->data = source + *assignFrom;
    Dot11s_ReadHdrFromElement(element, &ieHdr);

    if (assignLimit > 0)
    {
        ERROR_Assert(ieHdr.length + *assignFrom <= assignLimit,
            "Dot11s_AssignToElement: assignment exceeds limit.\n");
    }

    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;
    *assignFrom += element->length;
}

// ------------------------------------------------------------------

/**
FUNCTION   :: Dot11s_CreateMeshCapabilityElement
LAYER      :: MAC
PURPOSE    :: Create a Mesh Capability IE
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : IE to create
+ pathProfile : DOT11s_PathProfile : path protocol & metric
+ peerCapacity : int        : balance peer capacity
RETURN     :: void
NOTES      :: This IE contains a single active protocol and metric.
                It does not permit an annoucement of all protocols/metrics
                that the MP supports. There seems to be no other IE that
                can do so.
                Most of the fields are not used or implemented. If used,
                consider declaring a MeshCapabilityData that can be passed
                as a parameter instead of individual values.
**/

static
void Dot11s_CreateMeshCapabilityElement(
    Node* node,
    DOT11_Ie* element,
    const DOT11s_PathProfile* const pathProfile,
    int peerCapacity)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_MESH_CAPABILITY);
    Dot11s_AppendToElement(element, ieHdr);

    Dot11s_AppendToElement(element, DOT11s_IE_VERSION);

    Dot11s_AppendToElement(
        element, DOT11s_PathProtocolId[pathProfile->pathProtocol], 4);
    Dot11s_AppendToElement(
        element, DOT11s_PathMetricId[pathProfile->pathMetric], 4);

    // Unused fields set to 0 in constructor.
    DOT11s_PeerCapacityField aPeerCapacity;

    // Peer capacity is 13 bits, actually
    aPeerCapacity.peerCapacity = (unsigned short)(peerCapacity);
    aPeerCapacity.operatingAsMP = 1;
    Dot11s_AppendToElement(element, aPeerCapacity, 2);

    // Unused
    DOT11s_PowerSaveCapabilityField aPsCapability;
    Dot11s_AppendToElement(element, aPsCapability, 1);

    // Unused
    DOT11s_SynchCapabilityField aSynchCapability;
    Dot11s_AppendToElement(element, aSynchCapability, 1);

    // Unused
    DOT11s_MdaCapabilityField anMdaCapability;
    Dot11s_AppendToElement(element, anMdaCapability, 1);

    // Unused
    int aChannelPrecedance = 0;
    Dot11s_AppendToElement(element, aChannelPrecedance, 4);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParseMeshCapabilityElement
LAYER      :: MAC
PURPOSE    :: Parse a Mesh Capability IE
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ pathProfile : DOT11s_PathProfile* : parsed path protocol & metric
+ peerCapacity : int*       : balance peer capacity
RETURN     :: void
NOTES      :: Only the three currently used fields are read.
                Should mirror Dot11s_CreateMeshCapabilityElement().
**/

void Dot11s_ParseMeshCapabilityElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_PathProfile* pathProfile,
    int* peerCapacity)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_MESH_CAPABILITY,
        "Dot11s_ParseMeshCapabilityElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    unsigned char version;
    Dot11s_ReadFromElement(element, &ieOffset, &version, 1);
    ERROR_Assert(version == DOT11s_IE_VERSION,
        "Dot11s_ParseMeshCapabilityElement: "
        "IE has wrong version.\n");

    unsigned int activeProtocolId;
    unsigned int activeMetricId;
    Dot11s_ReadFromElement(element, &ieOffset, &activeProtocolId, 4);
    Dot11s_ReadFromElement(element, &ieOffset, &activeMetricId, 4);
    pathProfile->pathProtocol =
        Dot11s_GetPathProtocolFromId(activeProtocolId);
    pathProfile->pathMetric =
        Dot11s_GetPathMetricFromId(activeMetricId);

    DOT11s_PeerCapacityField aPeerCapacity;
    Dot11s_ReadFromElement(element, &ieOffset, &aPeerCapacity, 2);
    *peerCapacity = aPeerCapacity.peerCapacity;

    // Unused
    DOT11s_PowerSaveCapabilityField aPsCapability;
    Dot11s_ReadFromElement(element, &ieOffset, &aPsCapability, 1);

    // Unused
    DOT11s_SynchCapabilityField aSynchCapability;
    Dot11s_ReadFromElement(element, &ieOffset, &aSynchCapability, 1);

    // Unused
    DOT11s_MdaCapabilityField anMdaCapability;
    Dot11s_ReadFromElement(element, &ieOffset, &anMdaCapability, 1);

    // Unused
    int aChannelPrecedance = 0;
    Dot11s_ReadFromElement(element, &ieOffset, &aChannelPrecedance, 4);

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParseMeshCapabilityElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreateActiveProfileElement
LAYER      :: MAC
PURPOSE    :: Create Active Profile IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ pathProfile : DOT11s_PathProfile* : path protocol & metric
RETURN     :: void
**/

static
void Dot11s_CreateActiveProfileElement(
    Node* node,
    DOT11_Ie* element,
    const DOT11s_PathProfile* const pathProfile)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_ACTIVE_PROFILE_ANNOUNCEMENT);
    Dot11s_AppendToElement(element, ieHdr);

    Dot11s_AppendToElement(element, DOT11s_IE_VERSION);
    Dot11s_AppendToElement(
        element, DOT11s_PathProtocolId[pathProfile->pathProtocol], 4);
    Dot11s_AppendToElement(
        element, DOT11s_PathMetricId[pathProfile->pathMetric], 4);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParseActiveProfileElement
LAYER      :: MAC
PURPOSE    :: Parse Active Profile IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ pathProfile : DOT11s_PathProfile* : parsed path protocol & metric
RETURN     :: void
**/

void Dot11s_ParseActiveProfileElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_PathProfile* pathProfile)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_ACTIVE_PROFILE_ANNOUNCEMENT,
        "Dot11s_ParseActiveProfileElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    unsigned char version;
    Dot11s_ReadFromElement(element, &ieOffset, &version, 1);
    ERROR_Assert(version == DOT11s_IE_VERSION,
        "Dot11s_ParseActiveProfileElement: "
        "IE has wrong version.\n");

    unsigned int activeProtocolId;
    unsigned int activeMetricId;
    Dot11s_ReadFromElement(element, &ieOffset, &activeProtocolId, 4);
    Dot11s_ReadFromElement(element, &ieOffset, &activeMetricId, 4);
    pathProfile->pathProtocol =
        Dot11s_GetPathProtocolFromId(activeProtocolId);
    pathProfile->pathMetric =
        Dot11s_GetPathMetricFromId(activeMetricId);

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParseActiveProfileElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreateMeshIdElement
LAYER      :: MAC
PURPOSE    :: Create Mesh ID IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ meshId    : char*         : null terminated mesh ID
RETURN     :: void
**/

static
void Dot11s_CreateMeshIdElement(
    Node* node,
    DOT11_Ie* element,
    char* meshId)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_MESH_ID);
    Dot11s_AppendToElement(element, ieHdr);

    // Ensure that Mesh ID is no more than 32 chars.
    // As this is a string, Dot11s_AppendToElement()
    // cannot be used directly.
    int idLength = (int)strlen(meshId);
    idLength = MIN(idLength, DOT11s_MESH_ID_LENGTH_MAX);
    memcpy(element->data + element->length, meshId, (size_t)idLength);
    element->length += idLength;

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParseMeshIdElement
LAYER      :: MAC
PURPOSE    :: Parse Mesh ID IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ meshId    : char*         : parsed mesh ID with null terminator
RETURN     :: void
**/

void Dot11s_ParseMeshIdElement(
    Node* node,
    DOT11_Ie* element,
    char* meshId)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_MESH_ID,
        "Dot11s_ParseMeshIdElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    // Cannot use Dot11s_ReadFromElement() directly.
    ERROR_Assert(
        ieHdr.length <= DOT11s_MESH_ID_LENGTH_MAX,
        "Dot11s_ParseMeshIdElement: "
        "Mesh ID exceeds permissible length.\n");
    memcpy(meshId, element->data + ieOffset, ieHdr.length);
    meshId[ieHdr.length] = '\0';
    ieOffset += ieHdr.length;

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParseMeshIdElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreateLinkStateAnnouncementElement
LAYER      :: MAC
PURPOSE    :: Create Link State Announcement IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rateMbps  : int           : rate in Mpbs
+ per       : float         : PER in range [0, 1)
RETURN     :: void
NOTES      :: If PER is >= 1 it is decreased to largest permissibe value.
**/

static
void Dot11s_CreateLinkStateAnnouncementElement(
    Node* node,
    DOT11_Ie* element,
    int rateMbps,
    float per)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_LINK_STATE_ANNOUNCEMENT);
    Dot11s_AppendToElement(element, ieHdr);

    Dot11s_AppendToElement(element, (unsigned short)(rateMbps), 2);

    // Protect against PER = 1
    if (per >= 1.0f)
    {
        per = 1.0f - DOT11s_PER_MULTIPLE;
    }
    Dot11s_AppendToElement(
        element, (unsigned short)(per / DOT11s_PER_MULTIPLE), 2);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParseLinkStateAnnouncementElement
LAYER      :: MAC
PURPOSE    :: Parse Link State Announcement IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rateMbps  : int*          : parsed rate in Mbps
+ per       : float*        : parsed PER
RETURN     :: void
**/
void Dot11s_ParseLinkStateAnnouncementElement(
    Node* node,
    DOT11_Ie* element,
    int* rateMbps,
    float* per)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_LINK_STATE_ANNOUNCEMENT,
        "Dot11s_ParseLinkStateAnnouncementElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    unsigned short rate;
    Dot11s_ReadFromElement(element, &ieOffset, &rate, 2);
    *rateMbps = rate;

    unsigned short perMultiple;
    Dot11s_ReadFromElement(element, &ieOffset, &perMultiple, 2);
    *per = perMultiple * DOT11s_PER_MULTIPLE;

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParseLinkStateAnnouncementElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreateRreqElement
LAYER      :: MAC
PURPOSE    :: Create RREQ IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rreqData  : HWMP_RreqData*: RREQ data
RETURN     :: void
**/

static
void Dot11s_CreateRreqElement(
    Node* node,
    DOT11_Ie* element,
    const HWMP_RreqData* const rreqData)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_RREQ);
    Dot11s_AppendToElement(element, ieHdr);

    unsigned char aFlag = 0;
    aFlag |= (rreqData->portalRole ? 0x80 : 0);
    aFlag |= (rreqData->isBroadcast ? 0x40 : 0);
    aFlag |= (rreqData->rrepRequested ? 0x20 : 0);
    aFlag |= (rreqData->staReassocFlag ? 0x10 : 0);
    Dot11s_AppendToElement(element, aFlag, 1);

    Dot11s_AppendToElement(element, rreqData->hopCount, 1);
    Dot11s_AppendToElement(element, rreqData->ttl, 1);

    Dot11s_AppendToElement(element, rreqData->rreqId, 4);
    Dot11s_AppendToElement(element, rreqData->sourceInfo.addr, 6);
    Dot11s_AppendToElement(element, rreqData->sourceInfo.seqNo, 4);
    Dot11s_AppendToElement(element,
        (unsigned int) (rreqData->lifetime / MILLI_SECOND), 4);
    Dot11s_AppendToElement(element, rreqData->metric, 4);

    // Assumes 1 destination per RREQ
    // Change for multiple destFlag/destAddr/dsn
    unsigned char destCount = 1;
    Dot11s_AppendToElement(element, destCount, 1);

    unsigned char rreqFlag = 0;
    rreqFlag |= (rreqData->DO_Flag ? 0x80 : 0);
    rreqFlag |= (rreqData->RF_Flag ? 0x40 : 0);
    Dot11s_AppendToElement(element, rreqFlag, 1);
    Dot11s_AppendToElement(element, rreqData->destInfo.addr, 6);
    Dot11s_AppendToElement(element, rreqData->destInfo.seqNo, 4);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParseRreqElement
LAYER      :: MAC
PURPOSE    :: Parse RREQ IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rreqData  : HWMP_RreqData*: parsed RREQ data
RETURN     :: void
**/

void Dot11s_ParseRreqElement(
    Node* node,
    DOT11_Ie* element,
    HWMP_RreqData* rreqData)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_RREQ,
        "Dot11s_ParseRreqElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    unsigned char aFlag;
    Dot11s_ReadFromElement(element, &ieOffset, &aFlag, 1);
    rreqData->portalRole = (aFlag & 0x80 ? TRUE : FALSE);
    rreqData->isBroadcast = (aFlag & 0x40 ? TRUE : FALSE);
    rreqData->rrepRequested = (aFlag & 0x20 ? TRUE : FALSE);
    rreqData->staReassocFlag = (aFlag & 0x10 ? TRUE : FALSE);

    Dot11s_ReadFromElement(element, &ieOffset, &(rreqData->hopCount), 1);
    Dot11s_ReadFromElement(element, &ieOffset, &(rreqData->ttl), 1);

    Dot11s_ReadFromElement(element, &ieOffset, &(rreqData->rreqId), 4);
    Dot11s_ReadFromElement(element, &ieOffset,
        &(rreqData->sourceInfo.addr), 6);
    Dot11s_ReadFromElement(element, &ieOffset,
        &(rreqData->sourceInfo.seqNo), 4);

    unsigned int lifetime;
    Dot11s_ReadFromElement(element, &ieOffset, &lifetime, 4);
    rreqData->lifetime = lifetime * MILLI_SECOND;

    Dot11s_ReadFromElement(element, &ieOffset, &(rreqData->metric), 4);

    // Assumes single destination, else process via list
    unsigned char destCount;
    Dot11s_ReadFromElement(element, &ieOffset, &destCount, 1);
    ERROR_Assert(destCount == 1,
        "Dot11s_ParseRreqElement: Destination count should be 1.\n");

    unsigned char rreqFlag;
    Dot11s_ReadFromElement(element, &ieOffset, &rreqFlag, 1);
    rreqData->DO_Flag = (rreqFlag & 0x80 ? TRUE : FALSE);
    rreqData->RF_Flag = (rreqFlag & 0x40 ? TRUE : FALSE);
    Dot11s_ReadFromElement(
        element, &ieOffset, &(rreqData->destInfo.addr), 6);
    Dot11s_ReadFromElement(
        element, &ieOffset, &(rreqData->destInfo.seqNo), 4);

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParseRreqElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreateRrepElement
LAYER      :: MAC
PURPOSE    :: Create RREP IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rrepData  : HWMP_RrepData*: RREP data
RETURN     :: void
NOTES      :: The size limit of an IE does not permit an RREP
                to have more than HWMP_RREP_SOURCE_COUNT_MAX
                address/sequence number pairs.
                In a case of a gratuitous RREQ the limit is
                larger at HWMP_GRAT_RRREP_SOURCE_COUNT_MAX.
**/

static
void Dot11s_CreateRrepElement(
    Node* node,
    DOT11_Ie* element,
    const HWMP_RrepData* const rrepData)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_RREP);
    Dot11s_AppendToElement(element, ieHdr);

    unsigned char aFlag = 0;
    aFlag |= (rrepData->isGratuitous ? 0x80 : 0);
    Dot11s_AppendToElement(element, aFlag, 1);

    Dot11s_AppendToElement(element, rrepData->hopCount, 1);
    Dot11s_AppendToElement(element, rrepData->ttl, 1);

    Dot11s_AppendToElement(element, rrepData->destInfo.addr, 6);
    Dot11s_AppendToElement(element, rrepData->destInfo.seqNo, 4);

    Dot11s_AppendToElement(element,
        (unsigned int) (rrepData->lifetime / MILLI_SECOND), 4);
    Dot11s_AppendToElement(element, rrepData->metric, 4);

    int sourceCount = ListGetSize(node, rrepData->sourceInfoList);

    if (rrepData->isGratuitous == FALSE
        && sourceCount > HWMP_RREP_SOURCE_COUNT_MAX)
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr, "Dot11s_CreateRrepElement: "
            "An RREP IE can hold a max of %d source related fields.\n"
            "Handling more than this count requires code modification.\n"
            "The parameter has %d sources to be added to the RREP.\n",
            HWMP_RREP_SOURCE_COUNT_MAX,
            sourceCount);
        ERROR_ReportError(errorStr);
    }

    if (rrepData->isGratuitous == TRUE
        && sourceCount > HWMP_GRAT_RREP_SOURCE_COUNT_MAX)
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr, "Dot11s_CreateRrepElement: "
            "A gratuitous RREP IE can hold a max of %d source "
            "related fields.\n"
            "Handling more than this count requires code modification.\n"
            "The parameter has %d sources to be added to the RREP.\n",
            HWMP_GRAT_RREP_SOURCE_COUNT_MAX,
            sourceCount);
        ERROR_ReportError(errorStr);
    }

    Dot11s_AppendToElement(element, (unsigned char) sourceCount, 1);

    HWMP_AddrSeqInfo* sourceInfo;
    ListItem *item = rrepData->sourceInfoList->first;
    while (item)
    {
        sourceInfo = (HWMP_AddrSeqInfo*) item->data;

        Dot11s_AppendToElement(element, sourceInfo->addr, 6);

        if (rrepData->isGratuitous == FALSE)
        {
            Dot11s_AppendToElement(element, sourceInfo->seqNo, 4);
        }

        item = item->next;
    }

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParseRrepElement
LAYER      :: MAC
PURPOSE    :: Parse RREP IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rrepData  : HWMP_RrepData*: parsed RREP data
RETURN     :: void
**/

void Dot11s_ParseRrepElement(
    Node* node,
    DOT11_Ie* element,
    HWMP_RrepData* rrepData)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_RREP,
        "Dot11s_ParseRrepElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    unsigned char aFlag = 0;
    Dot11s_ReadFromElement(element, &ieOffset, &aFlag, 1);
    rrepData->isGratuitous = (aFlag & 0x80 ? TRUE : FALSE);

    Dot11s_ReadFromElement(element, &ieOffset, &(rrepData->hopCount), 1);
    Dot11s_ReadFromElement(element, &ieOffset, &(rrepData->ttl), 1);

    Dot11s_ReadFromElement(
        element, &ieOffset, &(rrepData->destInfo.addr), 6);
    Dot11s_ReadFromElement(
        element, &ieOffset, &(rrepData->destInfo.seqNo), 4);

    unsigned int lifetime;
    Dot11s_ReadFromElement(element, &ieOffset, &lifetime, 4);
    rrepData->lifetime = lifetime * MILLI_SECOND;

    Dot11s_ReadFromElement(element, &ieOffset, &(rrepData->metric), 4);

    unsigned char sourceCount;
    Dot11s_ReadFromElement(element, &ieOffset, &sourceCount, 1);

    HWMP_AddrSeqInfo* sourceInfo;
    for (int i = 0; i < sourceCount; i++)
    {
        Dot11s_Malloc(HWMP_AddrSeqInfo, sourceInfo);

        Dot11s_ReadFromElement(
            element, &ieOffset, &(sourceInfo->addr), 6);

        if (rrepData->isGratuitous == FALSE)
        {
            Dot11s_ReadFromElement(
                element, &ieOffset, &(sourceInfo->seqNo), 4);
        }
        else
        {
            sourceInfo->seqNo = 0;
        }

        ListAppend(node, rrepData->sourceInfoList, 0, sourceInfo);
    }

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParseRrepElement: all fields not parsed.\n");
}


/**
FUNCTION   :: Dot11s_CreateRerrElement
LAYER      :: MAC
PURPOSE    :: Create RERR IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rerrData  : HWMP_RerrData*: parsed RERR data
RETURN     :: void
**/

static
void Dot11s_CreateRerrElement(
    Node* node,
    DOT11_Ie* element,
    const HWMP_RerrData* const rerrData)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_RERR);
    Dot11s_AppendToElement(element, ieHdr);

    unsigned char aModeFlag = 0;
    Dot11s_AppendToElement(element, aModeFlag, 1);

    int destCount = ListGetSize(node, rerrData->destInfoList);

    if (destCount > HWMP_RERR_DEST_COUNT_MAX)
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr, "Dot11s_CreateRerrElement: "
            "An RERR IE can hold a max of %d destination related fields.\n"
            "The parameter has %d sources to be added to the RERR.\n",
            HWMP_RERR_DEST_COUNT_MAX,
            destCount);
        ERROR_ReportError(errorStr);
    }

    Dot11s_AppendToElement(element, (unsigned char) destCount, 1);

    HWMP_AddrSeqInfo* destInfo;
    ListItem *item = rerrData->destInfoList->first;
    while (item)
    {
        destInfo = (HWMP_AddrSeqInfo*) item->data;

        Dot11s_AppendToElement(element, destInfo->addr, 6);
        Dot11s_AppendToElement(element, destInfo->seqNo, 4);

        item = item->next;
    }

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParseRerrElement
LAYER      :: MAC
PURPOSE    :: Parse RERR IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rerrData  : HWMP_RerrData*: parsed RERR data
RETURN     :: void
**/

void Dot11s_ParseRerrElement(
    Node* node,
    DOT11_Ie* element,
    HWMP_RerrData* rerrData)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_RERR,
        "Dot11s_ParseRerrElement: IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    unsigned char aModeFlag;
    Dot11s_ReadFromElement(element, &ieOffset, &aModeFlag, 1);

    unsigned char destCount;
    Dot11s_ReadFromElement(element, &ieOffset, &destCount, 1);

    HWMP_AddrSeqInfo* destInfo;
    for (int i = 0; i < destCount; i++)
    {
        Dot11s_Malloc(HWMP_AddrSeqInfo, destInfo);

        Dot11s_ReadFromElement(
            element, &ieOffset, &(destInfo->addr), 6);
        Dot11s_ReadFromElement(
            element, &ieOffset, &(destInfo->seqNo), 4);

        ListAppend(node, rerrData->destInfoList, 0, destInfo);
    }

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParseRerrElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreatePeerLinkOpenElement
LAYER      :: MAC
PURPOSE    :: Create Peer Link Open IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ assocData : DOT11s_AssocData* : association data
RETURN     :: void
**/

static
void Dot11s_CreatePeerLinkOpenElement(
    Node* node,
    DOT11_Ie* element,
    const DOT11s_AssocData* const assocData)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_PEER_LINK_OPEN);
    Dot11s_AppendToElement(element, ieHdr);

    Dot11s_AppendToElement(element, assocData->linkId, 4);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParsePeerLinkOpenElement
LAYER      :: MAC
PURPOSE    :: Parse Peer Request IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ assocData : DOT11s_AssocData* : association data
RETURN     :: void
**/

void Dot11s_ParsePeerLinkOpenElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_AssocData* assocData)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_PEER_LINK_OPEN,
        "Dot11s_ParsePeerLinkOpenElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    Dot11s_ReadFromElement(element, &ieOffset, &(assocData->linkId), 4);

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParsePeerLinkOpenElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreatePeerLinkConfirmElement
LAYER      :: MAC
PURPOSE    :: Create Peer Link Confirm IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ assocData : DOT11s_AssocData* : association data
RETURN     :: void
**/

static
void Dot11s_CreatePeerLinkConfirmElement(
    Node* node,
    DOT11_Ie* element,
    const DOT11s_AssocData* const assocData)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_PEER_LINK_CONFIRM);
    Dot11s_AppendToElement(element, ieHdr);

    Dot11s_AppendToElement(element, assocData->linkId, 4);
    Dot11s_AppendToElement(element, assocData->peerLinkId, 4);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParsePeerLinkConfirmElement
LAYER      :: MAC
PURPOSE    :: Parse Peer Link Confirm IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ assocData : DOT11s_AssocData* : association data
RETURN     :: void
NOTES      ::
**/

void Dot11s_ParsePeerLinkConfirmElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_AssocData* assocData)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_PEER_LINK_CONFIRM,
        "Dot11s_ParsePeerLinkConfirmElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    Dot11s_ReadFromElement(element, &ieOffset,
        &(assocData->linkId), 4);
    Dot11s_ReadFromElement(element, &ieOffset,
        &(assocData->peerLinkId), 4);

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParsePeerLinkConfirmElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreatePeerLinkCloseElement
LAYER      :: MAC
PURPOSE    :: Create Peer Link Close IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ assocData : DOT11s_AssocData* : association data
RETURN     :: void
**/

static
void Dot11s_CreatePeerLinkCloseElement(
    Node* node,
    DOT11_Ie* element,
    const DOT11s_AssocData* const assocData)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_PEER_LINK_CLOSE);
    Dot11s_AppendToElement(element, ieHdr);

    Dot11s_AppendToElement(element, assocData->linkId, 4);
    Dot11s_AppendToElement(element, assocData->peerLinkId, 4);
    Dot11s_AppendToElement(element, assocData->status, 1);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParsePeerLinkCloseElement
LAYER      :: MAC
PURPOSE    :: Parse Peer Link Close IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ assocData : DOT11s_AssocData* : association data
RETURN     :: void
**/

void Dot11s_ParsePeerLinkCloseElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_AssocData* assocData)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_PEER_LINK_CLOSE,
        "Dot11s_ParsePeerLinkCloseElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    Dot11s_ReadFromElement(element, &ieOffset,
        &(assocData->linkId), 4);
    Dot11s_ReadFromElement(element, &ieOffset,
        &(assocData->peerLinkId), 4);
    Dot11s_ReadFromElement(element, &ieOffset,
        &(assocData->status), 1);

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParsePeerLinkCloseElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreatePortalReachabilityElement
LAYER      :: MAC
PURPOSE    :: Create Portal Reachability IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
RETURN     :: void
NOTES      :: Not currently used.
**/

static
void Dot11s_CreatePortalReachabilityElement(
    Node* node,
    DOT11_Ie* element,
    LinkedList* portalList)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_PORTAL_REACHABILITY);
    Dot11s_AppendToElement(element, ieHdr);

    unsigned char portalCount = 0;
    Dot11s_AppendToElement(element, portalCount, 1);

    //Dot11s_AppendToElement(element, portalAddr, 6);
    //Dot11s_AppendToElement(element, metric, 4);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParsePortalReachabilityElement
LAYER      :: MAC
PURPOSE    :: Parse Portal Reachability IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
RETURN     :: void
NOTES      :: Not currently used.
**/

void Dot11s_ParsePortalReachabilityElement(
    Node* node,
    DOT11_Ie* element,
    LinkedList* portalList)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_PORTAL_REACHABILITY,
        "Dot11s_ParsePortalReachabilityElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    unsigned char portalCount;
    Dot11s_ReadFromElement(element, &ieOffset, &portalCount, 1);

    //process via list (if list is not empty ?)
    //Dot11s_ReadFromElement(element, &ieOffset, &portalAddr, 6);
    //Dot11s_ReadFromElement(element, &ieOffset, &metric, 4);

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParsePortalReachabilityElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreateRannElement
LAYER      :: MAC
PURPOSE    :: Create Portal / Root Announcement IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rannData  : HWMP_RannData*: RANN data
RETURN     :: void
NOTES      :: Assumes single root portal without any connected portals.
**/

static
void Dot11s_CreateRannElement(
    Node* node,
    DOT11_Ie* element,
    const HWMP_RannData* const rannData)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_RANN);
    Dot11s_AppendToElement(element, ieHdr);

    unsigned char rannFlag = 0;
    rannFlag |= (rannData->isPortal ? 0x80 : 0);
    rannFlag |= (rannData->registration ? 0x40 : 0);
    Dot11s_AppendToElement(element, rannFlag, 1);

    Dot11s_AppendToElement(element, rannData->hopCount, 1);
    Dot11s_AppendToElement(element, rannData->ttl, 1);

    Dot11s_AppendToElement(element, rannData->rootAddr, 6);
    Dot11s_AppendToElement(element, rannData->rootSeqNo, 4);
    Dot11s_AppendToElement(element,
        (unsigned int) (rannData->lifetime / MILLI_SECOND), 4);
    Dot11s_AppendToElement(element, rannData->metric, 4);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParseRannElement
LAYER      :: MAC
PURPOSE    :: Parse Portal / Root Announcement IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ rannData  : HWMP_RannData*: parsed RANN data
RETURN     :: void
NOTES      :: Assumes no connected portal field.
**/

void Dot11s_ParseRannElement(
    Node* node,
    DOT11_Ie* element,
    HWMP_RannData* rannData)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_RANN,
        "Dot11s_ParseRannElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    unsigned char rannFlag;
    Dot11s_ReadFromElement(element, &ieOffset, &rannFlag, 1);
    rannData->isPortal = (rannFlag & 0x80);
    rannData->registration = (rannFlag & 0x40);

    Dot11s_ReadFromElement(
        element, &ieOffset, &(rannData->hopCount), 1);
    Dot11s_ReadFromElement(
        element, &ieOffset, &(rannData->ttl), 1);

    Dot11s_ReadFromElement(
        element, &ieOffset, &(rannData->rootAddr), 6);
    Dot11s_ReadFromElement(
        element, &ieOffset, &(rannData->rootSeqNo), 4);
    unsigned int aLifeTime;
    Dot11s_ReadFromElement(element, &ieOffset, &aLifeTime, 4);
    rannData->lifetime = aLifeTime * MILLI_SECOND;
    Dot11s_ReadFromElement(
        element, &ieOffset, &(rannData->metric), 4);

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParsePortalAnnouncementElement: all fields not parsed.\n") ;
}


/**
FUNCTION   :: Dot11s_CreateNeighborListElement
LAYER      :: MAC
PURPOSE    :: Create
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ neighborList : LinkedList* : list of neighbors
RETURN     :: void
NOTES      :: This IE is for DTIM beacon frames and is not currently used.
**/

static
void Dot11s_CreateNeighborListElement(
    Node* node,
    DOT11_Ie* element,
    LinkedList* neighborList)
{
    element->length = 0;

    int neighborCount = ListGetSize(node, neighborList);

    DOT11_IeHdr ieHdr(DOT11_IE_NEIGHBOR_LIST);
    Dot11s_AppendToElement(element, ieHdr);

    DOT11s_NeighborListMpControl mpControl;
    Dot11s_AppendToElement(element, mpControl, 1);

    // We add the count field. It is not in the standard.
    // The count field is consistent with other IEs that have a list
    Dot11s_AppendToElement(element, (unsigned char) neighborCount, 1);

    if (neighborCount > 0) {
        ListItem* listItem = neighborList->first;
        DOT11s_NeighborItem* neighborItem;

        while (listItem) {
            neighborItem = (DOT11s_NeighborItem*) listItem->data;
            Dot11s_AppendToElement(element, neighborItem->neighborAddr, 6);
            listItem = listItem->next;
        }
    }

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParseNeighborListElement
LAYER      :: MAC
PURPOSE    :: Parse Neighbor List IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ neighborList : LinkedList* : parsed neighbor list
RETURN     :: void
NOTES      :: Untested. Unused.
**/

void Dot11s_ParseNeighborListElement(
    Node* node,
    DOT11_Ie* element,
    LinkedList* neighborList)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_NEIGHBOR_LIST,
        "Dot11s_ParseNeighborListElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    DOT11s_NeighborListMpControl mpControl;
    Dot11s_ReadFromElement(element, &ieOffset, &mpControl, 1);

    // Added a count field. It is not in the draft.
    unsigned char neighborCount;
    Dot11s_ReadFromElement(element, &ieOffset, &neighborCount, 1);

    if (neighborCount > 0) {
        Mac802Address neighborAddr;
        for (int i = 0; i < neighborCount; i++) {
            Dot11s_ReadFromElement(element, &ieOffset, &neighborAddr, 6);

            DOT11s_NeighborItem* neighborItem;
            Dot11s_MallocMemset0(DOT11s_NeighborItem, neighborItem);
            neighborItem->neighborAddr = neighborAddr;
            ListAppend(node, neighborList, 0, neighborItem);
        }
    }

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParseNeighborListElement: all fields not parsed.\n") ;
}

/**
FUNCTION   :: Dot11s_CreatePannElement
LAYER      :: MAC
PURPOSE    :: Create Portal Announcement IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ pannData  : DOT11s_PannData*: PANN data
RETURN     :: void
**/

static
void Dot11s_CreatePannElement(
    Node* node,
    DOT11_Ie* element,
    const DOT11s_PannData* const pannData)
{
    element->length = 0;

    DOT11_IeHdr ieHdr(DOT11_IE_PANN);
    Dot11s_AppendToElement(element, ieHdr);

    unsigned char aFlag = 0;                // unused
    Dot11s_AppendToElement(element, aFlag, 1);

    Dot11s_AppendToElement(element, pannData->hopCount, 1);
    Dot11s_AppendToElement(element, pannData->ttl, 1);

    Dot11s_AppendToElement(element, pannData->portalAddr, 6);
    Dot11s_AppendToElement(element, pannData->portalSeqNo, 4);
    Dot11s_AppendToElement(element, pannData->metric, 4);

    Dot11s_StoreIeLength(element);
}


/**
FUNCTION   :: Dot11s_ParsePannElement
LAYER      :: MAC
PURPOSE    :: Parse Portal Announcement IE.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ element   : DOT11_Ie*     : pointer to IE
+ pannData  : DOT11s_PannData*: parsed PANN data
RETURN     :: void
**/

void Dot11s_ParsePannElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_PannData* pannData)
{
    int ieOffset = 0;

    DOT11_IeHdr ieHdr;
    Dot11s_ReadFromElement(element, &ieOffset, &ieHdr);
    ERROR_Assert(
        ieHdr.id == DOT11_IE_PANN,
        "Dot11s_ParsePannElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    unsigned char aFlag;                // unused
    Dot11s_ReadFromElement(element, &ieOffset, &aFlag, 1);

    Dot11s_ReadFromElement(
        element, &ieOffset, &(pannData->hopCount), 1);
    Dot11s_ReadFromElement(
        element, &ieOffset, &(pannData->ttl), 1);

    Dot11s_ReadFromElement(
        element, &ieOffset, &(pannData->portalAddr), 6);
    Dot11s_ReadFromElement(
        element, &ieOffset, &(pannData->portalSeqNo), 4);
    Dot11s_ReadFromElement(
        element, &ieOffset, &(pannData->metric), 4);

    ERROR_Assert(ieOffset == element->length,
        "Dot11s_ParsePortalAnnouncementElement: all fields not parsed.\n") ;
}



/**
FUNCTION   :: Dot11s_CreateBeaconFrame
LAYER      :: MAC
PURPOSE    :: Create a beacon frame.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ element   : DOT11_Ie*     : pointer to IE
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
NOTES      :: This function partially creates the beacon frame.
                It adds mesh related elements. Fixed length fields
                are filled in separately.
**/

void Dot11s_CreateBeaconFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Ie* element,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    DOT11s_Data* mp = dot11->mp;
    *txFrameSize = sizeof(DOT11_BeaconFrame);
    element->data = (unsigned char*)txFrame + *txFrameSize;

    //Omit OFDM Parameter set

    Dot11s_CreateMeshIdElement(node, element, mp->meshId);
    *txFrameSize += element->length;

    element->data = (unsigned char*)txFrame + *txFrameSize;
    DOT11s_PathProfile pathProfile;
    pathProfile.pathProtocol = mp->pathProtocol;
    pathProfile.pathMetric = mp->pathMetric;
    Dot11s_CreateMeshCapabilityElement(node, element,
        &pathProfile, mp->peerCapacity);
    *txFrameSize += element->length;
}


/**
FUNCTION   :: Dot11s_CreateAssocRequestFrame
LAYER      :: MAC
PURPOSE    :: Create Association Request frame
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ assocData : DOT11s_AssocData* : association data
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
NOTES      :: This function currently adds mesh related fields.
                Fixed length fields are filled in by the calling
                function.
**/

void Dot11s_CreateAssocRequestFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_AssocData* assocData,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    DOT11s_Data* mp = dot11->mp;

    // Skip over fixed length fields.
    *txFrameSize = sizeof(DOT11_AssocReqFrame);

    DOT11_Ie* element;
    Dot11s_Malloc(DOT11_Ie, element);

    element->data = (unsigned char*)txFrame + *txFrameSize;
    Dot11s_CreateMeshIdElement(node, element, mp->meshId);
    *txFrameSize += element->length;

    element->data = (unsigned char*)txFrame + *txFrameSize;
    DOT11s_PathProfile pathProfile;
    pathProfile.pathProtocol = mp->pathProtocol;
    pathProfile.pathMetric = mp->pathMetric;
    Dot11s_CreateMeshCapabilityElement(node, element,
        &pathProfile, mp->peerCapacity);
    *txFrameSize += element->length;

    element->data = (unsigned char*)txFrame + *txFrameSize;
    Dot11s_CreateActiveProfileElement(node, element, &pathProfile);
    *txFrameSize += element->length;

    // Peer link open IE
    element->data = (unsigned char*)txFrame + *txFrameSize;
    Dot11s_CreatePeerLinkOpenElement(node, element, assocData);
    *txFrameSize += element->length;

    MEM_free(element);
}


/**
FUNCTION   :: Dot11s_CreateAssocResponseFrame
LAYER      :: MAC
PURPOSE    :: Create an association response frame.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ assocData : DOT11s_AssocData* : association data
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
NOTES      :: This function currently adds mesh related fields.
                Fixed length fields are filled in by the calling
                function.
**/

void Dot11s_CreateAssocResponseFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_AssocData* assocData,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    DOT11s_Data* mp = dot11->mp;

    // Skip over fixed length fields.
    *txFrameSize = sizeof(DOT11_AssocRespFrame);

    DOT11_Ie* element;
    Dot11s_Malloc(DOT11_Ie, element);

    element->data = (unsigned char*)txFrame + *txFrameSize;
    Dot11s_CreateMeshIdElement(node, element, mp->meshId);
    *txFrameSize += element->length;

    element->data = (unsigned char*)txFrame + *txFrameSize;
    DOT11s_PathProfile pathProfile;
    pathProfile.pathProtocol = mp->pathProtocol;
    pathProfile.pathMetric = mp->pathMetric;
    Dot11s_CreateMeshCapabilityElement(node, element,
        &pathProfile, mp->peerCapacity);
    *txFrameSize += element->length;

    element->data = (unsigned char*)txFrame + *txFrameSize;
    Dot11s_CreateActiveProfileElement(node, element, &pathProfile);
    *txFrameSize += element->length;

    // Peer link Confirm IE
    element->data = (unsigned char*)txFrame + *txFrameSize;
    Dot11s_CreatePeerLinkConfirmElement(node, element, assocData);
    *txFrameSize += element->length;

    MEM_free(element);
}


/**
FUNCTION   :: Dot11s_CreateAssocCloseFrame
LAYER      :: MAC
PURPOSE    :: Create disassociate frame to close a mesh peer link
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ assocData : DOT11s_AssocData* : association data
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
**/

void Dot11s_CreateAssocCloseFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11s_AssocData* assocData,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    *txFrameSize = sizeof(DOT11_DisassocFrame);

    DOT11_Ie* element;
    Dot11s_Malloc(DOT11_Ie, element);

    // Peer link Close IE
    element->data = (unsigned char*)txFrame + *txFrameSize;
    Dot11s_CreatePeerLinkCloseElement(node, element, assocData);
    *txFrameSize += element->length;

    MEM_free(element);
}


/**
FUNCTION   :: Dot11s_CreateLinkStateFrame
LAYER      :: MAC
PURPOSE    :: Create a mesh action frame with link state IE
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ neighborItem : DOT11s_NeighborItem* : Mesh neighbor data
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
**/

void Dot11s_CreateLinkStateFrame(
    Node* node,
    MacDataDot11* dot11,
    const DOT11s_NeighborItem* const neighborItem,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    // Skip over header
    *txFrameSize = sizeof(DOT11_FrameHdr);
    unsigned char* txFrameCast = (unsigned char*) txFrame;

    // Add category and mesh action
    *(txFrameCast + *txFrameSize) = (unsigned char)DOT11_ACTION_MESH;
    (*txFrameSize)++;
    *(txFrameCast + *txFrameSize) =
        (unsigned char) DOT11_MESH_LINK_STATE_ANNOUNCEMENT;
    (*txFrameSize)++;

    DOT11_Ie* element;
    Dot11s_Malloc(DOT11_Ie, element);
    element->data = (unsigned char*)txFrame + *txFrameSize;

    Dot11s_CreateLinkStateAnnouncementElement(node, element,
        neighborItem->dataRateInMbps, neighborItem->PER);
    *txFrameSize += element->length;

    MEM_free(element);
}


/**
FUNCTION   :: Dot11s_CreateRreqFrame
LAYER      :: MAC
PURPOSE    :: Create a mesh action frame with RREQ IE
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ rreqData  : HWMP_RreqData* : RREQ data
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
**/

void Dot11s_CreateRreqFrame(
    Node* node,
    MacDataDot11* dot11,
    const HWMP_RreqData* const rreqData,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    // Skip over header
    *txFrameSize = sizeof(DOT11_FrameHdr);
    unsigned char* txFrameCast = (unsigned char*) txFrame;

    // Add category
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_ACTION_MESH;
    (*txFrameSize)++;

    // Add mesh action
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_MESH_RREQ;
    (*txFrameSize)++;

    DOT11_Ie* element;
    Dot11s_Malloc(DOT11_Ie, element);
    element->data = (unsigned char*)txFrame + *txFrameSize;

    Dot11s_CreateRreqElement(node, element, rreqData);

    *txFrameSize += element->length;

    MEM_free(element);
}


/**
FUNCTION   :: Dot11s_CreateRrepFrame
LAYER      :: MAC
PURPOSE    :: Create a mesh action frame with RREP IE
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ rrepData  : HWMP_RrepData* : RREP data
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
**/

void Dot11s_CreateRrepFrame(
    Node* node,
    MacDataDot11* dot11,
    const HWMP_RrepData* const rrepData,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    // Skip over header
    *txFrameSize = sizeof(DOT11_FrameHdr);
    unsigned char* txFrameCast = (unsigned char*) txFrame;

    // Add category and action
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_ACTION_MESH;
    (*txFrameSize)++;
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_MESH_RREP;
    (*txFrameSize)++;

    DOT11_Ie* element;
    Dot11s_Malloc(DOT11_Ie, element);
    element->data = (unsigned char*)txFrame + *txFrameSize;

    Dot11s_CreateRrepElement(node, element, rrepData);

    *txFrameSize += element->length;

    MEM_free(element);
}


/**
FUNCTION   :: Dot11s_CreateRerrFrame
LAYER      :: MAC
PURPOSE    :: Create a mesh action frame with RERR IE
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ rerrData  : HWMP_RerrData* : RERR data
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
**/

void Dot11s_CreateRerrFrame(
    Node* node,
    MacDataDot11* dot11,
    const HWMP_RerrData* const rerrData,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    // Skip over header
    *txFrameSize = sizeof(DOT11_FrameHdr);
    unsigned char* txFrameCast = (unsigned char*) txFrame;

    // Add category and mesh action
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_ACTION_MESH;
    (*txFrameSize)++;
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_MESH_RERR;
    (*txFrameSize)++;

    DOT11_Ie* element;
    Dot11s_Malloc(DOT11_Ie, element);
    element->data = (unsigned char*)txFrame + *txFrameSize;

    Dot11s_CreateRerrElement(node, element, rerrData);

    *txFrameSize += element->length;

    MEM_free(element);
}


/**
FUNCTION   :: Dot11s_CreateRannFrame
LAYER      :: MAC
PURPOSE    :: Create a mesh action frame with RANN IE
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ rannData  : HWMP_RannData* : RANN data
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
**/

void Dot11s_CreateRannFrame(
    Node* node,
    MacDataDot11* dot11,
    const HWMP_RannData* const rannData,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    // Skip over header
    *txFrameSize = sizeof(DOT11_FrameHdr);
    unsigned char* txFrameCast = (unsigned char*) txFrame;

    // Add category and action
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_ACTION_MESH;
    (*txFrameSize)++;
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_MESH_RANN;
    (*txFrameSize)++;

    DOT11_Ie* element;
    Dot11s_Malloc(DOT11_Ie, element);
    element->data = (unsigned char*)txFrame + *txFrameSize;

    Dot11s_CreateRannElement(node, element, rannData);

    *txFrameSize += element->length;

    MEM_free(element);
}


/**
FUNCTION   :: Dot11s_CreatePannFrame
LAYER      :: MAC
PURPOSE    :: Create a mesh action frame with PANN IE
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ pannData  : DOT11s_PannData* : PANN data
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
**/

void Dot11s_CreatePannFrame(
    Node* node,
    MacDataDot11* dot11,
    const DOT11s_PannData* const pannData,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    // Skip over header
    *txFrameSize = sizeof(DOT11_FrameHdr);
    unsigned char* txFrameCast = (unsigned char*) txFrame;

    // Add category and mesh action
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_ACTION_MESH;
    (*txFrameSize)++;
    *(txFrameCast + *txFrameSize) = (unsigned char) DOT11_MESH_PANN;
    (*txFrameSize)++;

    DOT11_Ie* element;
    Dot11s_Malloc(DOT11_Ie, element);
    element->data = (unsigned char*)txFrame + *txFrameSize;

    Dot11s_CreatePannElement(node, element, pannData);

    *txFrameSize += element->length;

    MEM_free(element);
}

// ------------------------------------------------------------------

// Utility fn to estimate beacon duration at different rates.
// This value is used by an MP to adjust its beacon offset
// to minimize collision with neighbors.
// Estimate gives about 892 us for 802.11b at 2 Mbps.
// The code replaces this with a fixed value of 1000 micro-seconds
// to check for collision.
clocktype Dot11s_GetBeaconDuration(
    Node* node,
    MacDataDot11* dot11)
{
    int size = 0;

    // Add fixed length fields
    size += sizeof(DOT11_BeaconFrame);

    DOT11_Ie element;
    DOT11_IeHdr ieHdr;

    char meshId[DOT11s_MESH_ID_LENGTH_MAX + 1];
    memset(meshId, 0, DOT11s_MESH_ID_LENGTH_MAX + 1);
    Dot11s_CreateMeshIdElement(node, &element, meshId);
    size += element.data[1];

    DOT11s_PathProfile pathProfile;
    pathProfile.pathProtocol = DOT11s_PATH_PROTOCOL_HWMP;
    pathProfile.pathMetric = DOT11s_PATH_METRIC_AIRTIME;
    Dot11s_CreateMeshCapabilityElement(node, &element,
        &pathProfile, 255);
    size += element.data[1];

    //Dot11s_CreateNeighborListElement(
    //    node, &element, dot11->mp->neighborList);
    //size += element.data[1] + 5 * 6;    //assume 5 neighbors

    //Dot11s_CreatePortalReachabilityElement(node, &element,
    //    dot11->mp->portalList);
    size += element.data[1] + 23;        //assume 2 portal

    return PHY_GetTransmissionDuration(
                node, dot11->myMacData->phyNumber,
                //insert desired data rata type here
                dot11->highestDataRateTypeForBC,
                size);
}

// ------------------------------------------------------------------
