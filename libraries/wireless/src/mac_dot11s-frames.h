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


#ifndef MAC_DOT11S_FRAMES_H
#define MAC_DOT11S_FRAMES_H

// ------------------------------------------------------------------
/*
802.11s related frame handling. Follows IEEE 802.11s
Draft 0, Revision 2 (P802.11s-D0.02.pdf) of May 2006
and subsequent modifications upto Revision 6 a.k.a.
Version 1 of December 2006. Some frames are likely
to have further changes, noted in their relevant places.

The code relates to
     - Mesh Information Elements (IEs)
     - Mesh Frames

Each implemented IE has a Create<Name>Element() and
Parse<Name>Element() that codes the specifics of
the fields such as bit packing, address as 4 or 6
bytes, time in required units, etc.

Similarly, each frame has a Create<Name>Frame() that
calls the corresponding IEs creation functions and
appends them to the frame in the required order. There
is, currently, no Parse<Name>Frame() set of functions.

*/

// Forward declarations
struct  MacDataDot11;
struct  DOT11_FrameHdr;
struct  DOT11_MacFrame;
struct  DOT11s_NeighborItem;
struct  DOT11s_PathProfile;

// ------------------------------------------------------------------

/**
DEFINE      :: DOT11_IE_LENGTH_MAX
DESCRIPTION :: Maximum length of an Information Element.
                IEs consist of
                - 1 byte type
                - 1 byte length
                - 255 bytes body
**/
#define DOT11_IE_LENGTH_MAX                 257

/**
DEFINE      :: DOT11_IE_HDR_LENGTH
DESCRIPTION :: Length of IE header
**/
#define DOT11_IE_HDR_LENGTH                 2

/**
DEFINE      :: DOT11_IE_HDR_LENGTH_OFFSET
DESCRIPTION :: Offset of length field in the IE
**/
#define DOT11_IE_HDR_LENGTH_OFFSET          1

/**
DEFINE      :: DOT11_ADDR_PADDING
DESCRIPTION :: A padding for MAC address fields.
                NodeAddress is 4 bytes. An extra 2 bytes
                needs to be added to conform to MAC 6 byte
                address fields. To use, if required.
**/
#define DOT11_ADDR_PADDING                  ((short)0)

/**
DEFINE      :: DOT11s_DATA_FRAME_HDR_SIZE
DESCRIPTION :: Size of mesh data frame header, without QoS control.
**/
#define DOT11s_DATA_FRAME_HDR_SIZE          38

/**
DEFINE      :: DOT11s_MESH_ID_LENGTH_MAX
DESCRIPTION :: Maximum length of Mesh ID field.
                The length does not include a trailing \0.
**/
#define DOT11s_MESH_ID_LENGTH_MAX           32

/**
DEFINE      :: DOT11s_PER_MULTIPLE
DESCRIPTION :: Packet error rate (PER) is encoded as 2 bytes
                in Local Link State announcement IEs, as
                multiples of this granular value.
**/
#define DOT11s_PER_MULTIPLE                 (1.0f/65536)

/**
DEFINE      :: DOT11s_IE_VERSION
DESCRIPTION :: Version field as used in many IEs.
                For example, Mesh Capability IE.
**/
#define DOT11s_IE_VERSION                   ((unsigned char) 1)

/**
DEFINE      :: DOT11s_SSID_WILDCARD
DESCRIPTION :: Wildcard SSID used in beacon frames by non-AP
                mesh points to avoid stations attempting to
                associate.
                A 0 length value is proposed.
**/
#define DOT11s_SSID_WILDCARD        "RESERVED: MESH POINT OPERATIONS"

/**
DEFINE      :: HWMP_RREQ_FIXED_LENGTH
DESCRIPTION :: Fixed length portion of RREQ IE. Includes type
                and length. Assumes address is 4 bytes.
                Note: Fig s107 does not show a destination
                count field of 1 byte.
**/
#define HWMP_RREQ_FIXED_LENGTH              28

/**
DEFINE      :: HWMP_RREQ_PER_DEST_LENGTH
DESCRIPTION :: Per destination length in RREQ IE.
                Assumes address is 4 bytes.
**/
#define HWMP_RREQ_PER_DEST_LENGTH           11

/**
DEFINE      :: HWMP_RREQ_DEST_COUNT_MAX
DESCRIPTION :: Max destinations per RREQ.
**/
#define HWMP_RREQ_DEST_COUNT_MAX            \
    ((DOT11_IE_LENGTH_MAX                   \
        - HWMP_RREQ_FIXED_LENGTH)           \
    / HWMP_RREQ_PER_DEST_LENGTH)

/**
DEFINE      :: HWMP_RREP_FIXED_LENGTH
DESCRIPTION :: Fixed length portion of RREP IE.
                Includes type and length
                Assumes address is 4 bytes.
                Note: Fig s108 does not show a source
                count field of 1 byte.
**/
#define HWMP_RREP_FIXED_LENGTH              24

/**
DEFINE      :: HWMP_RREP_PER_SOURCE_LENGTH
DESCRIPTION :: Per source length in RREP IE.
                Assumes address is 4 bytes.
**/
#define HWMP_RREP_PER_SOURCE_LENGTH         10

/**
DEFINE      :: HWMP_RREP_SOURCE_COUNT_MAX
DESCRIPTION :: Max. sources per RREP IE.
**/
#define HWMP_RREP_SOURCE_COUNT_MAX          \
    ((DOT11_IE_LENGTH_MAX                   \
        - HWMP_RREP_FIXED_LENGTH)           \
    / HWMP_RREP_PER_SOURCE_LENGTH)

/**
DEFINE      :: HWMP_GRAT_RREP_PER_SOURCE_LENGTH
DESCRIPTION :: Per source length for a gratuitous RREP IE.
                Assumes address is 4 bytes.
**/
#define HWMP_GRAT_RREP_PER_SOURCE_LENGTH         6

/**
DEFINE      :: HWMP_GRAT_RREP_SOURCE_COUNT_MAX
DESCRIPTION :: Max. sources per RREP IE.
**/
#define HWMP_GRAT_RREP_SOURCE_COUNT_MAX     \
    ((DOT11_IE_LENGTH_MAX                   \
        - HWMP_RREP_FIXED_LENGTH)           \
    / HWMP_GRAT_RREP_PER_SOURCE_LENGTH)


/**
DEFINE      :: HWMP_RERR_FIXED_LENGTH
DESCRIPTION :: Fixed length portion of RERR IE.
                Includes type and length.
                Assumes destination count field is 1 byte.
**/
#define HWMP_RERR_FIXED_LENGTH              4

/**
DEFINE      :: HWMP_RERR_PER_DEST_LENGTH
DESCRIPTION :: Per destination length in RERR IE.
                Assumes address is 4 bytes.
**/
#define HWMP_RERR_PER_DEST_LENGTH           10

/**
DEFINE      :: HWMP_RERR_DEST_COUNT_MAX
DESCRIPTION :: Max destinations per RERR.
**/
#define HWMP_RERR_DEST_COUNT_MAX            \
    ((DOT11_IE_LENGTH_MAX                   \
        - HWMP_RERR_FIXED_LENGTH)           \
    / HWMP_RERR_PER_DEST_LENGTH)

// ------------------------------------------------------------------

/**
ENUM        :: DOT11_IeId
DESCRIPTION :: ID of Mesh Information Elements.
                Values are not specified in the draft.
                They are assumed to begin from 51.
                Some of these IEs are deprecated.
**/
enum DOT11_IeId
{
    DOT11_IE_MESH_CAPABILITY                = 51,
    DOT11_IE_PATH_SELECTION_PROTOCOL,
    DOT11_IE_PATH_SELECTION_METRIC,
    DOT11_IE_ACTIVE_PROFILE_ANNOUNCEMENT,
    DOT11_IE_MESH_ID,

    DOT11_IE_LINK_STATE_ANNOUNCEMENT,       //56
    DOT11_IE_RREQ,
    DOT11_IE_RREP,
    DOT11_IE_RERR,
    DOT11_IE_RREP_ACK,                      //deprecated

    DOT11_IE_OFDM_PARAMETER,                //61
    DOT11_IE_TRANSMISSION_RATE,
    DOT11_IE_TRAFFIC_LOAD,
    DOT11_IE_NEIGHBORHOOD_CONGESTION,
    DOT11_IE_PEER_REQUEST,                  //deprecated

    DOT11_IE_PEER_RESPONSE,                 //66
    DOT11_IE_PORTAL_REACHABILITY,
    DOT11_IE_RANN,
    DOT11_IE_UCG_SWITCH,
    DOT11_IE_NEIGHBOR_LIST,

    DOT11_IE_PEER_LINK_CLOSE,
    DOT11_IE_PEER_LINK_OPEN,
    DOT11_IE_PEER_LINK_CONFIRM,
    DOT11_IE_PANN,

    DOT11_IE_RESERVED
};

/**
ENUM        :: DOT11_ActionCategory
DESCRIPTION :: ID of Action category in Action frames.
                Mesh action category is assumed to be 5.
                A new Mesh frame type with subtype Mesh
                Action has been introduced in Draft 6. The
                new frame uses 4 addresses with mesh control.
                The category values used here refer to 3
                address management action frames of type 0x0D.
**/
enum DOT11_ActionCategory
{
    DOT11_ACTION_SPECTRUM,
    DOT11_ACTION_QOS,
    DOT11_ACTION_DLS,
    DOT11_ACTION_BLOCK_ACK,
    DOT11_ACTION_MESH                       = 5,

    DOT11_ACTION_RESERVED                   = 255
};

/**
ENUM        :: DOT11s_MeshFieldId
DESCRIPTION :: ID of Mesh fields within Mesh Category.
                IDs are assumed; marked TBD in the draft.
**/
enum DOT11s_MeshFieldId
{
    DOT11_MESH_LINK_STATE_ANNOUNCEMENT      = 0,
    DOT11_MESH_PEER_LINK_DISCONNECT,                // unused
    DOT11_MESH_RREQ,
    DOT11_MESH_RREP,
    DOT11_MESH_RERR,
    DOT11_MESH_RREP_ACK                     = 5,    // unused

    DOT11_MESH_RANN                         = 16,
    DOT11_MESH_PANN                         = 17,

    DOT11_MESH_RESERVED                     = 255
};

// ------------------------------------------------------------------

/**
STRUCT      :: DOT11_IeHdr
DESCRIPTION :: Information Element header.
                Consists of type, length of 1 byte each.
**/
struct DOT11_IeHdr
{
    unsigned char id;
    unsigned char length;

    DOT11_IeHdr(unsigned char theId = 0)
        :   id(theId), length(0)
    { }

    void operator= (unsigned char* data)
    {
        id = *data;
        length = *(data + 1);
    }

};

/**
STRUCT      :: DOT11_Ie
DESCRIPTION :: Utility structure for working with IEs.
                Length consists of entire IE, including header.
**/
struct DOT11_Ie
{
    int length;
    unsigned char* data;
};

/**
STRUCT      :: DOT11s_FwdControl
DESCRIPTION :: Mesh Forwarding Control field in mesh data frame headers.
                This is actually 3 bytes in size.
                - E2E sequence number is assumed to be 22 bits,
                not 16 bits, and does not wraparound.
                - UsesTbr is true for Tree forwarding.
                This flag is not in the draft.
                - ExMesh flag is true for non-mesh source address.
                This flag is not in the draft header specs.
NOTES       :: The size and layout of the "mesh header" has been
                revised in Draft 4 along with potentially two
                additional address fields. It is proposed that the
                mesh header be an additional header to the 802.11
                header and part of the payload. This has not yet
                been implemented.
**/
struct DOT11s_FwdControl
{
    unsigned int fwdControl;

    DOT11s_FwdControl(void)
        :   fwdControl(0)
    {}

    DOT11s_FwdControl(
        unsigned int ttl,
        unsigned int e2eSeqNo,
        BOOL usesTbr,
        BOOL exMesh)
    {
        ERROR_Assert(ttl <= 0xFF,
            "DOT11s_FwdControl: "
            "TTL is 8 bit. Should be less than 256.\n");

        ERROR_Assert(e2eSeqNo <= 0x3FFFFF,
            "DOT11s_FwdControl: "
            "Sequence number should be less than Ox003FFFFF.\n");

        fwdControl =
            (ttl)
            | (e2eSeqNo << 8)
            | ((usesTbr ? 1 : 0) << 30)
            | ((exMesh ? 1 : 0) << 31);
    }

    unsigned int GetTTL()
    {
        return (fwdControl & 0xFF);
    }

    void SetTTL(unsigned int ttl)
    {
        ERROR_Assert(ttl <= 0xFF,
            "DOT11s_FwdControl::SetTTL "
            "TTL is 8 bit. Should be less than 256.\n");

        fwdControl = (fwdControl & 0xFFFFFF00) | (ttl);
    }

    unsigned int GetE2eSeqNo()
    {
        return (fwdControl & 0x3FFFFF00) >> 8;
    }

    void SetE2eSeqNo(unsigned int e2eSeqNo)
    {
        ERROR_Assert(e2eSeqNo <= 0x3FFFFF,
            "DOT11s_FwdControl::SetE2eSeqNo "
            "Seq No. should be less than 0x003FFFFF.\n");

        fwdControl = (fwdControl & 0xC00000FF) | (e2eSeqNo << 8);
    }

    BOOL GetUsesTbr()
    {
        return (fwdControl & 0x40000000) ? TRUE : FALSE;
    }

    void SetUsesTbr(BOOL usesTbr)
    {
        fwdControl = (fwdControl & 0xBFFFFFFF)
                     | ((usesTbr ? 1 : 0) << 30);
    }

    BOOL GetExMesh()
    {
        return (fwdControl & 0x80000000) ? TRUE : FALSE;
    }

    void SetExMesh(BOOL exMesh)
    {
        fwdControl = (fwdControl & 0x7FFFFFFF)
                      | ((exMesh ? 1 : 0) << 31);
    }

    void operator= (unsigned int value)
    {
        fwdControl = value;
    }

};

/**
STRUCT      :: DOT11s_FrameHdr
DESCRIPTION :: Mesh 4 address frame header, without QoS fields.
**/
struct DOT11s_FrameHdr
{
                                  //  Should Be  Actually
    unsigned char   frameType;    //      1         1
    unsigned char   frameFlags;   //      1         1
    UInt16          duration;     //      2         2
    Mac802Address   destAddr;     //      6         6
    Mac802Address   sourceAddr;   //      6         6
    Mac802Address   address3;     //      6         6
    UInt16          fragId: 4,    //      2         2
                    seqNo: 12 ;   //
    Mac802Address   address4;     //      6         6
    //Two byte padding            //      0         2
    DOT11s_FwdControl fwdControl; //      4         4
    //char            FCS[0];       //      4         0
                                  //---------------------
                                  //     38        36
};

/**
STRUCT      :: DOT11s_FrameInfoActionData
DESCRIPTION :: Utility structure for FrameInfo data to help
                identify action frame subtypes.
**/

struct DOT11s_FrameInfoActionData
{
    DOT11_ActionCategory category;
    DOT11s_MeshFieldId fieldId;

    DOT11s_FrameInfoActionData()
        : category(DOT11_ACTION_RESERVED),
          fieldId(DOT11_MESH_RESERVED)
    {}

    void Init()
    {
        category = DOT11_ACTION_RESERVED;
        fieldId = DOT11_MESH_RESERVED;
    }
};

/**
STRUCT      :: DOT11s_FrameInfo
DESCRIPTION :: Items in frame buffer/queues. Used for both
                management & data queues. Holds all relevant
                information to create the appropriate 3 address
                or 4 address data or management frame, except
                for duration and sequence number.
                Here, the RA/TA/DA/SA fields are the logical values
                and may not correspond to what is written to the
                address1/2/3/4 frame fields.
**/
struct DOT11s_FrameInfo
{
    Message* msg;
    unsigned char frameType;
    Mac802Address RA;         //Receiving address
    Mac802Address TA;         //Transmitting address
    Mac802Address DA;         //Final destination
    Mac802Address SA;         //Initial source
    DOT11s_FwdControl fwdControl;
    clocktype insertTime;
    DOT11s_FrameInfoActionData actionData;
    //priority

    DOT11s_FrameInfo()
        :   msg(NULL), frameType(0x3F /* Reserved frame type */),
            insertTime(0)
    {
        RA = INVALID_802ADDRESS;
        TA = INVALID_802ADDRESS;
        DA = INVALID_802ADDRESS;
        SA = INVALID_802ADDRESS;
    }

    void Init()
    {
        msg = NULL; frameType = 0x3F;
        RA = INVALID_802ADDRESS; TA = INVALID_802ADDRESS;
        DA = INVALID_802ADDRESS; SA = INVALID_802ADDRESS;
        insertTime = 0;
    }
};


/**
STRUCT      :: DOT11s_PeerCapacityField
DESCRIPTION :: Peer Capacity field used in mesh capability IE.
                Assumes 0..n.
**/
struct DOT11s_PeerCapacityField
{
    unsigned short
        peerCapacity:13,
        connectedToAS:1,
        simpleUnification:1,
        operatingAsMP:1;

    DOT11s_PeerCapacityField()
        :   peerCapacity(0), connectedToAS(0),
            simpleUnification(0), operatingAsMP(0)
        { }
};

/**
STRUCT      :: DOT11s_PowerSaveCapabilityField
DESCRIPTION :: Power Save Capability field used in mesh capability IE.
                Assumes 0..n encoding.
**/
struct DOT11s_PowerSaveCapabilityField
{
    unsigned char
        reserved:5,
        currentPowerSaveMode:1,
        requirePowerSaveSupport:1,
        supportingPowerSave:1;
    DOT11s_PowerSaveCapabilityField()
        :   reserved(0), currentPowerSaveMode(0),
            requirePowerSaveSupport(0), supportingPowerSave(0)
        { }
};

/**
STRUCT      :: DOT11s_SynchCapabilityField
DESCRIPTION :: Synchronization Capability field used in mesh capability IE.
**/
struct DOT11s_SynchCapabilityField
{
    unsigned char
        supportingSynch:1,
        requestsSynch:1,
        synching:1,
        reserved:5;
    DOT11s_SynchCapabilityField()
        :   supportingSynch(0), requestsSynch(0),
            synching(0)
        { }
};

/**
STRUCT      :: DOT11s_MdaCapabilityField
DESCRIPTION :: MDA Capability field used in mesh capability IE.
                Assumed 1 byte, though marked as two bytes in draft.
**/
struct DOT11s_MdaCapabilityField
{
    unsigned char
        mdaCapable:1,
        mdaActive:1,
        mdaActiveRequested:1,
        mdaNotAllowed:1,
        mdaEdcaMixedMode:1,
        reserved:3;
    DOT11s_MdaCapabilityField()
        :   mdaCapable(0), mdaActive(0),
            mdaActiveRequested(0), mdaNotAllowed(0),
            mdaEdcaMixedMode(0), reserved(0)
        {}
};

/**
STRUCT      :: DOT11s_AssocData
DESCRIPTION :: Association related data values.
                The open, confirm and close IEs use some or all members.
**/
struct DOT11s_AssocData
{
    unsigned int linkId;
    unsigned int peerLinkId;
    unsigned char status;
};

/**
STRUCT      :: DOT11s_PannData
DESCRIPTION :: Utility structure for Portal Announcements.
**/
struct DOT11s_PannData
{
    unsigned char hopCount;
    unsigned char ttl;
    Mac802Address portalAddr;
    int portalSeqNo;
    unsigned int metric;

};

/**
STRUCT      :: HWMP_AddrSeqInfo
DESCRIPTION :: Utility structure for IEs containing an address
                and sequence number pair.
**/
struct HWMP_AddrSeqInfo
{
    Mac802Address addr;
    int seqNo;
};

/**
STRUCT      :: HWMP_RreqData
DESCRIPTION :: Utility structure for RREQ IE.
                Assumes single destination per RREQ.
                If multiple destinations are required, change structure
                and handling to be similar to RREPs (HWMP_RrepData).
                The station reassociation flag is non-standard.
**/
struct HWMP_RreqData
{
    BOOL portalRole;
    BOOL isBroadcast;
    BOOL rrepRequested;
    BOOL staReassocFlag;
    unsigned char hopCount;
    unsigned char ttl;
    int rreqId;
    HWMP_AddrSeqInfo sourceInfo;
    clocktype lifetime;
    unsigned int metric;

    unsigned char destCount;        // not in Fig s107; fields mention it
    BOOL DO_Flag;
    BOOL RF_Flag;
    HWMP_AddrSeqInfo destInfo;
};

/**
STRUCT      :: HWMP_RrepData
DESCRIPTION :: Utility structure for RREP IE.
                Handles multiple sources / destination per RREP
                when gratuitous flag is set.
**/
struct HWMP_RrepData
{
    BOOL isGratuitous;              // Not in draft.
    unsigned char hopCount;
    unsigned char ttl;
    HWMP_AddrSeqInfo destInfo;
    clocktype lifetime;
    unsigned int metric;

    LinkedList* sourceInfoList;
};

/**
STRUCT      :: HWMP_RerrData
DESCRIPTION :: Utility structure for RERR IE.
**/
struct HWMP_RerrData
{
    LinkedList* destInfoList;
};

/**
STRUCT      :: HWMP_RannData
DESCRIPTION :: Utility structure for RANN.
                Fields differ from those in the draft.
**/
struct HWMP_RannData
{
    BOOL isPortal;
    BOOL registration;              // for symmetry with proactive RREQ
    unsigned char hopCount;
    unsigned char ttl;
    Mac802Address rootAddr;
    int rootSeqNo;
    clocktype lifetime;
    unsigned int metric;

    void Reset()
    {
        rootAddr = INVALID_802ADDRESS;
    }
};

/**
STRUCT      :: DOT11s_NeighborListMpControl
DESCRIPTION :: MP Control field used in Neighbor List IE.
                Not currently implemented.
**/
struct DOT11s_NeighborListMpControl
{
    unsigned char
        reserved:5,
        designatedBB:1,
        bbSwitch:1,
        bbPSstate:1;
    DOT11s_NeighborListMpControl()
        :   reserved(0), designatedBB(0),
            bbSwitch(0), bbPSstate(0)
        {}
};


// ------------------------------------------------------------------

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
    int assignLimit);

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
NOTES      :: This function creates a partial beacon frame.
                It adds mesh related elements. Fixed length fields
                are filled in separately.
**/
void Dot11s_CreateBeaconFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Ie* element,
    DOT11_MacFrame* txFrame,
    int* txFrameSize);

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
    int* txFrameSize);

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
    int* txFrameSize);

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
    int* txFrameSize);

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
    int* txFrameSize);

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
    int* txFrameSize);

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
    int* txFrameSize);

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
    const HWMP_RerrData* const rrerData,
    DOT11_MacFrame* txFrame,
    int* txFrameSize);

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
    int* txFrameSize);


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
    int* txFrameSize);

// ------------------------------------------------------------------

void Dot11s_ParseMeshCapabilityElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_PathProfile* pathProfile,
    int* peerCapacity);

void Dot11s_ParseActiveProfileElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_PathProfile* pathProfile);

void Dot11s_ParseMeshIdElement(
    Node* node,
    DOT11_Ie* element,
    char* meshId);

void Dot11s_ParseLinkStateAnnouncementElement(
    Node* node,
    DOT11_Ie* element,
    int* r,
    float* e);

void Dot11s_ParseRreqElement(
    Node* node,
    DOT11_Ie* element,
    HWMP_RreqData* rreqData);

void Dot11s_ParseRrepElement(
    Node* node,
    DOT11_Ie* element,
    HWMP_RrepData* rrepData);

void Dot11s_ParseRerrElement(
    Node* node,
    DOT11_Ie* element,
    HWMP_RerrData* rerrData);

void Dot11s_ParsePeerLinkOpenElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_AssocData* assocData);

void Dot11s_ParsePeerLinkConfirmElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_AssocData* assocData);

void Dot11s_ParsePeerLinkCloseElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_AssocData* assocData);

void Dot11s_ParsePortalReachabilityElement(
    Node* node,
    DOT11_Ie* element,
    LinkedList* portalList);

void Dot11s_ParseRannElement(
    Node* node,
    DOT11_Ie* element,
    HWMP_RannData* rannData);

void Dot11s_ParseNeighborListElement(
    Node* node,
    DOT11_Ie* element,
    LinkedList* neighborList);

void Dot11s_ParsePannElement(
    Node* node,
    DOT11_Ie* element,
    DOT11s_PannData* pannData);


// ------------------------------------------------------------------

#endif // MAC_DOT11S_FRAMES_H
