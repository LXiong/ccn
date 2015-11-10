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


#ifndef RTP_H
#define RTP_H

#include "types.h"
#include "list.h"

#define RTP_VERSION             2
#define RTP_PACKET_HEADER_SIZE  ((sizeof(char *) * 2) + \
                                sizeof(unsigned int *) + (2 * sizeof(int)))

#define RTP_MAX_PACKET_LEN      1500
#define RTP_INVALID_PORT        0xFFFF
#define RTP_INVALID_SSRC        0xFFFF

#define RTP_MAX_DROPOUT         3000
#define RTP_MAX_MISORDER        100
#define RTP_MIN_SEQUENTIAL      2
#define RTP_MAX_CNAME_LEN       255
#define RTP_SEQ_MOD             0x10000
#define RTP_MAX_SDES_LEN        256
#define RTP_LOWER_LAYER_OVERHEAD 28 // IPv4 + UDP
#define RTP_CSRC_SIZE 4

// The size of the hash table used to hold the source database.
// Should be large enough that we're unlikely to get collisions
// when sources are added, but not too large that we waste too
// much memory. For now we assume around 100 participants is a sensible
// limit so we set this to 11.
#define RTP_DB_SIZE 11

// NTP timestamp calculation related stuff
#define RTP_SECS_BETWEEN_1900_1970  2208988800u

#define RTP_VERSION             2

// Minimum average time between RTCP packets from this site (in
// seconds).  This time prevents the reports from `clumping' when
// sessions are small and the law of large numbers isn't helping
// to smooth out the traffic.  It also keeps the report interval
// from becoming ridiculously small during transient outages like
// a network partition.
#define RTCP_MIN_TIME   5.0

// Fraction of the RTCP bandwidth to be shared among active
// senders.  (This fraction was chosen so that in a typical
// session with one or two active senders, the computed report
// time would be roughly equal to the minimum report time so that
// we don't unnecessarily slow down receiver reports.) The
// receiver fraction must be 1 - the sender fraction.
#define RTCP_SENDER_BW_FRACTION 0.25
#define RTCP_RCVR_BW_FRACTION   (1 - RTCP_SENDER_BW_FRACTION)

// To compensate for "unconditional reconsideration" converging
// to a value below the intended average.
#define RTCP_COMPENSATION    (2.71828 - 1.5)

#define RTCP_SR_FIXED_SIZE      28
#define RTCP_RR_BLOCK_SIZE      6
#define RTCP_RR_BLOCK_IN_BYTE   24
#define RTCP_MAX_NUM_BLOCK      31
#define DEFAULT_SESSION_MANAGEMENT_BANDWIDTH 64000



#define DEFAULT_JITTERBUFFER_MAXNO_PACKET  100
#define DEFAULT_JITTER_MAX_DELAY (10 * MILLI_SECOND)
#define DEFAULT_JITTER_TALKSPURT_DELAY (10 * MILLI_SECOND)
#define RTCP_VOIP_JITTER_DEBUG 0

//------------------------------Jitter Buffer struct-----------------------
typedef struct rtcp_jitter_buffer
{
    LinkedList*     listJitterNodes;   //Jitter Buffer Node List
    int             maxNoNodesInJitter;   //Maximum Number of Jitter Nodes
    unsigned short  expectedSeq;        //Expected Sequence for retrival
    clocktype       nominalDelay;       //Nominal Delay time for searching
                                        // Expected Sequence no Packet
    clocktype       maxDelay;           //Maximum Delay time for waiting
                                        // in Jitter List
    clocktype      talkspurtDelay;      //talk Spurt Delay
    clocktype      packetDelay;            //Packet Delay
    clocktype      packetVariance;          //Packet Variance

    unsigned int   packetDropped;      // Number of packet dropped due to
                                        // jitter Buffer implementation
    unsigned int   maxConsecutivePacketDropped;      // Maximum Number of
                                        // packet consecutive dropped due to
                                        // jitter Buffer implementation
    BOOL            firstPacket;        //Whether it is first packet, required
                                        // to despatch Nominal delay timer for
                                        // the first time
    Message*        msgNominalDelay;    //Message Nominal Delay
    Message*        msgTalkSpurtDelay;  //Message Talk spurt Delay

}RtpJitterBuffer;



//-------------------------RTCP Packet Description Section-------------------

//
//  SR: Sender report RTCP packet structure
//  RtcpSrPacket structure
//  The sender report packet consists of three sections, possibly
//  followed by a fourth profile-specific extension section if defined.
//  The first section, the header, is 8 octets long.
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |V=2|P|    RC   |   PT=SR=200   |             length            | header
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                         SSRC of sender                        |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |              NTP timestamp, most significant word             | sender
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ info
// |             NTP timestamp, least significant word             |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                         RTP timestamp                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                     sender's packet count                     |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      sender's octet count                     |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                 SSRC_1 (SSRC of first source)                 | report
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
// | fraction lost |       cumulative number of packets lost       |   1
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |           extended highest sequence number received           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      interarrival jitter                      |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                         last SR (LSR)                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                   delay since last SR (DLSR)                  |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                 SSRC_2 (SSRC of second source)                | report
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
// :                               ...                             :   2
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                  profile-specific extensions                  |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

typedef struct rtcp_sr_packet_struct
{
    unsigned int    ssrc;
    unsigned int    ntpSec;
    unsigned int    ntpFrac;
    unsigned int    rtptimeStamp;
    unsigned int    senderPktCount;
    unsigned int    senderByteCount;
} RtcpSrPacket;


//  RR: Receiver report RTCP packet format
//
//  The format of the receiver report (RR) packet is the same as that of
//  the SR packet except that the packet type field contains the constant
//  201 and the five words of sender information are omitted (these are
//  the NTP and RTP timestamps and sender's packet and octet counts). The
//  remaining fields have the same meaning as for the SR packet.
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |V=2|P|    RC   |   PT=RR=201   |             length            | header
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                     SSRC of packet sender                     |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                 SSRC_1 (SSRC of first source)                 | report
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
// | fraction lost |       cumulative number of packets lost       |   1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |           extended highest sequence number received           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      interarrival jitter                      |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                         last SR (LSR)                         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                   delay since last SR (DLSR)                  |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                 SSRC_2 (SSRC of second source)                | report
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
// :                               ...                             :   2
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                  profile-specific extensions                  |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
typedef struct rtcp_rr_packet_struct
{
    unsigned int    ssrc;       /* The ssrc to which this RR pertains */
    UInt32    RrPktLost;//fracLost:8;
                    //totLost:24;
    unsigned int    lastSeqNum;
    unsigned int    jitter;
    unsigned int    lsr;
    unsigned int    dlsr;
} RtcpRrPacket;

//-------------------------------------------------------------------------
// NAME         : RtcpRrPacketSetFracLost()
// PURPOSE      : Set the value of fracLost for RtcpRrPacket
// PARAMETERS   :
//    RrPktLost : The variable containing the value of fracLost and totLost
//    fracLost  : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpRrPacketSetFracLost(UInt32 *RrPktLost, UInt32 fracLost)
{
    //masks fracLost within boundry range
    fracLost = fracLost & maskInt(25, 32);

    //clears first 8 bits
    *RrPktLost = *RrPktLost & maskInt(9, 32);

    //Setting the value of fracLost in RrPktLost
    *RrPktLost = *RrPktLost | LshiftInt(fracLost, 8);
}


//-------------------------------------------------------------------------
// NAME         : RtcpRrPacketSetTotLost()
// PURPOSE      : Set the value of totLost for RtcpRrPacket
// PARAMETERS   :
//    RrPktLost : The variable containing the value of fracLost and totLost
//      totLost : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpRrPacketSetTotLost(UInt32 *RrPktLost, UInt32 totLost)
{
    //masks totLost within boundry range
    totLost = totLost & maskInt(9, 32);

    //clears all bits except first 8
    *RrPktLost = *RrPktLost & maskInt(1, 8);

    //setting the value of totlost in RrPktLost
    *RrPktLost = *RrPktLost | totLost;
}


//-------------------------------------------------------------------------
// NAME         : RtcpRrPacketGetFracLost()
// PURPOSE      : Returns the value of fracLost for RtcpRrPacket
// PARAMETERS   :
//    RrPktLost : The variable containing the value of fracLost and totLost
// RETURN VALUE : UInt32
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 RtcpRrPacketGetFracLost(UInt32 RrPktLost)
{
    UInt32 fracLost = RrPktLost;

    //clears all bits except first 8
    fracLost = fracLost & maskInt(1, 8);

    //right shift 24 places so that last 8 bits represent fracLost
    fracLost = RshiftInt(fracLost, 8);

    return fracLost;
}


//-------------------------------------------------------------------------
// NAME         : RtcpRrPacketGetTotLost()
// PURPOSE      : Returns the value of totLost for RtcpRrPacket
// PARAMETERS   :
//    RrPktLost : The variable containing the value of fracLost and totLost
// RETURN VALUE : UInt32
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 RtcpRrPacketGetTotLost(UInt32 RrPktLost)
{
    UInt32 totlost = RrPktLost;

    //clears all bits except first 8
    totlost = totlost & maskInt(9, 32);

    return totlost;
}


//  APP: Application-defined RTCP packet format
//
//  The APP packet is intended for experimental use as new applications
//  and new features are developed, without requiring packet type value
//  registration. APP packets with unrecognized names should be ignored.
//  After testing and if wider use is justified, it is recommended that
//  each APP packet be redefined without the subtype and name fields and
//  registered with the Internet Assigned Numbers Authority using an RTCP
//  packet type.
//
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |V=2|P| subtype |   PT=APP=204  |             length            |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                           SSRC/CSRC                           |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                          name (ASCII)                         |
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  |                   application-dependent data                 ...
//  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
typedef struct rtcp_app_packet_struct
{
    UInt16  rappPkt;
                    //version:2;  // RTP version
                    //p:1,        // padding flag
                    //subtype:5,  // application dependent
                    //pt:8;       // packet type
    unsigned short  length;     // packet length
    unsigned int    ssrc;       // packet ssrc
    char            name[4];    // four ASCII characters
    char            data[1];    // variable length field
} RtcpAppPacket;

//-------------------------------------------------------------------------
// NAME         : RtcpAppPacketSetVersion()
// PURPOSE      : Set the value of version for RtcpAppPacket
// PARAMETERS   :
//      rappPkt : The variable containing the value of version,p,subtype and
//                pt
//     version  : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpAppPacketSetVersion(UInt16 *rappPkt, UInt16 version)
{
    //masks version within boundry range
    version = version & maskShort(15, 16);

    //clear the first two bits
    *rappPkt = *rappPkt & maskShort(3, 16);

    //Setting the value of version in rappPkt
    *rappPkt = *rappPkt | LshiftShort(version, 2);
}


//-------------------------------------------------------------------------
// NAME         : RtcpAppPacketSetP()
// PURPOSE      : Set the value of p for RtcpAppPacket
// PARAMETERS   :
//      rappPkt : The variable containing the value of version,p,subtype and
//                pt
//            p : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpAppPacketSetP(UInt16 *rappPkt, BOOL p)
{
    UInt16 x = (UInt16)p;

    //masks p within boundry range
    x = x & maskShort(16, 16);

    //clears the third bit
    *rappPkt = *rappPkt & (~(maskShort(3, 3)));

    //Setting the value of p in rappPkt
    *rappPkt = *rappPkt | LshiftShort(x, 3);
}


//-------------------------------------------------------------------------
// NAME         : RtcpAppPacketSetSubType()
// PURPOSE      : Set the value of subtype for RtcpAppPacket
// PARAMETERS   :
//      rappPkt : The variable containing the value of version,p,subtype and
//                pt
//      subtype : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpAppPacketSetSubType(UInt16 *rappPkt, UInt16 subtype)
{
    //masks subtype within boundry range
    subtype = subtype & maskShort(12, 16);

    //clears bits 4-8
    *rappPkt = *rappPkt & (~(maskShort(4, 8)));

    //Setting the value of p in rappPkt
    *rappPkt = *rappPkt | LshiftShort(subtype, 8);
}


//-------------------------------------------------------------------------
// NAME         : RtcpAppPacketSetPt()
// PURPOSE      : Set the value of pt for RtcpAppPacket
// PARAMETERS   :
//      rappPkt : The variable containing the value of version,p,subtype and
//                pt
//           pt : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpAppPacketSetPt(UInt16 *rappPkt, unsigned char pt)
{
    UInt16 pt_short = (UInt16)pt;

    //masks pt within boundry range
    pt_short = pt_short & maskShort(9, 16);

    //clears the bits 9-16
    *rappPkt = *rappPkt & maskShort(1, 8);

    //Setting the value of p in rappPkt
    *rappPkt = *rappPkt | pt_short;
}


//-------------------------------------------------------------------------
// NAME         : RtcpAppPacketGetVersion()
// PURPOSE      : Returns the value of version for RtcpAppPacket
// PARAMETERS   :
//      rappPkt : The variable containing the value of version,p,subtype and
//                pt
// RETURN VALUE : UInt16
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 RtcpAppPacketGetVersion(UInt16 rappPkt)
{
    UInt16 version = rappPkt;

    //clears all the bits except first two
    version = version & maskShort(1, 2);

    //right shifts so that last two bits represent version
    version = RshiftShort(version, 2);

    return version;
}


//-------------------------------------------------------------------------
// NAME         : RtcpAppPacketGetP()
// PURPOSE      : Returns the value of p for RtcpAppPacket
// PARAMETERS   :
//      rappPkt : The variable containing the value of version,p,subtype and
//                pt
// RETURN VALUE : BOOL
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL RtcpAppPacketGetP(UInt16 rappPkt)
{
    UInt16 p = rappPkt;

    //clears all the bits except third
    p = p & maskShort(3, 3);

    //right shifts so that last bit represent p
    p = RshiftShort(p, 3);

    return (BOOL)p;
}


//-------------------------------------------------------------------------
// NAME         : RtcpAppPacketGetSubType()
// PURPOSE      : Returns the value of subtype for RtcpAppPacket
// PARAMETERS   :
//      rappPkt : The variable containing the value of version,p,subtype and
//                pt
// RETURN VALUE : UInt16
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 RtcpAppPacketGetSubType(UInt16 rappPkt)
{
    UInt16 subtype = rappPkt;

    //clears all the bits except 4-8
    subtype = subtype & maskShort(4, 8);

    //right shifts so that last 4 bits represent subtype
    subtype = RshiftShort(subtype, 8);

    return subtype;
}


//-------------------------------------------------------------------------
// NAME         : RtcpAppPacketGetPt()
// PURPOSE      : Returns the value of pt for RtcpAppPacket
// PARAMETERS   :
//      rappPkt : The variable containing the value of version,p,subtype and
//                pt
// RETURN VALUE : unsigned char
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static unsigned char RtcpAppPacketGetPt(UInt16 rappPkt)
{
    UInt16 pt = rappPkt;

    //clears first 8 bits
    pt = pt & maskShort(9, 16);

    return (unsigned char) pt;
}


// RTCP packet type
// abbrev.    name                     value
// SR         sender report            200
// RR         receiver report          201
// SDES       source description       202
// BYE        goodbye                  203
// APP        application-defined      204
typedef enum rtcp_packet_type
{
    RTCP_SR   = 200,
    RTCP_RR   = 201,
    RTCP_SDES = 202,
    RTCP_BYE  = 203,
    RTCP_APP  = 204
} RtcpPacketType;


// SDES packet types
typedef enum rtcp_sdes_type
{
    RTCP_SDES_END   = 0,
    RTCP_SDES_CNAME = 1,
    RTCP_SDES_NAME  = 2,
    RTCP_SDES_EMAIL = 3,
    RTCP_SDES_PHONE = 4,
    RTCP_SDES_LOC   = 5,
    RTCP_SDES_TOOL  = 6,
    RTCP_SDES_NOTE  = 7,
    RTCP_SDES_PRIV  = 8
} RtcpSdesType;

//  RTCP SDES item
//  SDES: Source description RTCP packet
//  The SDES packet is a three-level structure composed of a header and
//  zero or more chunks, each of of which is composed of items describing
//  the source identified in that chunk.
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|V=2|P|    SC   |  PT=SDES=202  |             length            | header
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//|                          SSRC/CSRC_1                          | chunk
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   1
//|                           SDES items                          |
//|                              ...                              |
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//|                          SSRC/CSRC_2                          | chunk
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   2
//|                           SDES items                          |
//|                              ...                              |
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+

typedef struct rtcp_sdes_item_struct
{
    unsigned char     type;    // type of SDES item
    unsigned char     length;  // length of SDES item (in bytes)
    char              data[1]; // text, not zero-terminated
} RtcpSdesItem;

struct rtcp_sdes_struct
{
    unsigned int            ssrc;
    RtcpSdesItem            item[1]; // list of SDES
};


//--------------------------------------------------------------------------
// RTCP packet structure

// RTCP packet: A control packet consisting of a fixed header part similar
//              to that of RTP data packets, followed by structured elements
//              that vary depending upon the RTCP packet type. Typically,
//              multiple RTCP packets are sent together as a compound RTCP
//              packet in a single packet of the underlying protocol; this
//              is enabled by the length field in the fixed header of each
//              RTCP packet.
//--------------------------------------------------------------------------

// RTCP common header
typedef struct rtcp_common_header_struct
{
    UInt16  rtcpCh;
                    //version:2;  // packet type
                    //p:1;        // padding flag
                    //count:5;    // varies by payload type
                    //pt:8;       // payload type
    unsigned short  length;     // packet length
} RtcpCommonHeader;

//-------------------------------------------------------------------------
// NAME         : RtcpCommonHeaderSetVersion()
// PURPOSE      : Set the value of version for RtcpCommonHeader
// PARAMETERS   :
//       rtcpCh : The variable containing the value of version,p,count and pt
//      version : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpCommonHeaderSetVersion(UInt16 *rtcpCh, UInt16 version)
{
    //masks version within boundry range
    version = version & maskShort(15, 16);

    //clears  the first two bits
    *rtcpCh = *rtcpCh & maskShort(3, 16);

    //setting the value of version in rtcpCh
    *rtcpCh = *rtcpCh | LshiftShort(version, 2);
}


//-------------------------------------------------------------------------
// NAME         : RtcpCommonHeaderSetP()
// PURPOSE      : Set the value of p for RtcpCommonHeader
// PARAMETERS   :
//       rtcpCh : The variable containing the value of version,p,count and pt
//            p : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpCommonHeaderSetP(UInt16 *rtcpCh, BOOL p)
{
    UInt16 x = (UInt16)p;

    //masks p within boundry range
    x = x & maskShort(16, 16);

    //clears the third bit
    *rtcpCh = *rtcpCh & (~(maskShort(3, 3)));

    //setting the value of p in rtcpCh
    *rtcpCh = *rtcpCh | LshiftShort(x, 3);
}


//-------------------------------------------------------------------------
// NAME         : RtcpCommonHeaderSetCount()
// PURPOSE      : Set the value of count for RtcpCommonHeader
// PARAMETERS   :
//       rtcpCh : The variable containing the value of version,p,count and pt
//        count : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpCommonHeaderSetCount(UInt16 *rtcpCh, UInt32 count)
{
    UInt16 count_short = (UInt16)count;

    //masks count within boundry range
    count_short = count_short & maskShort(12, 16);

    //clears the bits 4-8
    *rtcpCh = *rtcpCh & (~(maskShort(4, 8)));

    //setting the value of count in rtcpCh
    *rtcpCh = *rtcpCh | LshiftShort(count_short, 8);
}


//-------------------------------------------------------------------------
// NAME         : RtcpCommonHeaderSetPt()
// PURPOSE      : Set the value of pt for RtcpCommonHeader
// PARAMETERS   :
//       rtcpCh : The variable containing the value of version,p,count and pt
//           pt : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtcpCommonHeaderSetPt(UInt16 *rtcpCh, unsigned char pt)
{
    UInt16 pt_short = (UInt16)pt;

    //masks pt within boundry range
    pt_short = pt_short & maskShort(9, 16);

    //clears bits 9-16
    *rtcpCh = *rtcpCh & maskShort(1, 8);

    //setting the value of pt in rtcpCh
    *rtcpCh = *rtcpCh | pt_short;
}


//-------------------------------------------------------------------------
// NAME         : RtcpCommonHeaderGetVersion()
// PURPOSE      : Returns the value of version for RtcpCommonHeader
// PARAMETERS   :
//       rtcpCh : The variable containing the value of version,p,count and pt
// RETURN VALUE : UInt16
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 RtcpCommonHeaderGetVersion(UInt16 rtcpCh)
{
    UInt16 version = rtcpCh;

    //clears all the bits except first two
    version = version & maskShort(1, 2);

    //right shifts so that last two bits represent version
    version = RshiftShort(version, 2);

    return version;
}


//-------------------------------------------------------------------------
// NAME         : RtcpCommonHeaderGetP()
// PURPOSE      : Returns the value of p for RtcpCommonHeader
// PARAMETERS   :
//       rtcpCh : The variable containing the value of version,p,count and pt
// RETURN VALUE : BOOL
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL RtcpCommonHeaderGetP(UInt16 rtcpCh)
{
    UInt16 p = rtcpCh;

    //clears the third bits
    p = p & maskShort(3, 3);

    //right shifts so that last bit represents p
    p = RshiftShort(p, 3);

    return (BOOL)p;
}


//-------------------------------------------------------------------------
// NAME         : RtcpCommonHeaderGetCount()
// PURPOSE      : Returns the value of count for RtcpCommonHeader
// PARAMETERS   :
//       rtcpCh : The variable containing the value of version,p,count and pt
// RETURN VALUE : UInt16
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt16 RtcpCommonHeaderGetCount(UInt16 rtcpCh)
{
    UInt16 count = rtcpCh;

    //clears the bits 4-8
    count = count & maskShort(4, 8);

    //right shifts so that last 5 bits represents count
    count = RshiftShort(count, 8);

    return count;
}


//-------------------------------------------------------------------------
// NAME         : RtcpCommonHeaderGetPt()
// PURPOSE      : Returns the value of pt for RtcpCommonHeader
// PARAMETERS   :
//       rtcpCh : The variable containing the value of version,p,count and pt
// RETURN VALUE : unsigned char
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static unsigned char RtcpCommonHeaderGetPt(UInt16 rtcpCh)
{
    UInt16 pt = rtcpCh;

    //clears the first 8 bits
    pt = pt & maskShort(9, 16);

    return (unsigned char) pt;
}



typedef struct rtcp_packet_struct
{
    RtcpCommonHeader   common;
    union
    {
        struct
        {
            RtcpSrPacket                   sr;
            RtcpRrPacket                   rr[1]; // variable-length list
        } sr;
        struct
        {
            // source SSRC of this RTCP packet
            unsigned int              ssrc;
            RtcpRrPacket              rr[1]; // variable-length list
        } rr;
        //struct rtcp_sdes_struct
        //{
        //    unsigned int            ssrc;
        //    RtcpSdesItem            item[1]; // list of SDES
        //} sdes;
        struct rtcp_sdes_struct sdes;
        struct
        {
            // list of sources , can not express the trailing text
            unsigned int             ssrc[1];
        } bye;
        struct
        {
            unsigned int              ssrc;
            unsigned char             name[4];
            unsigned char             data[1];
        } app;
    } rtcpPacket;
} RtcpPacket;

// RTCP packet statistics structures,
// basically maintains count of different
// type of RTCP packets exchanged in a
// particular session
typedef struct
{
    unsigned int    rtcpSRSent;
    unsigned int    rtcpSRRecvd;
    unsigned int    rtcpRRSent;
    unsigned int    rtcpRRRecvd;
    unsigned int    rtcpSDESSent;
    unsigned int    rtcpSDESRecvd;
    unsigned int    rtcpBYESent;
    unsigned int    rtcpBYERecvd;
    unsigned int    rtcpAPPSent;
    unsigned int    rtcpAPPRecvd;
} RtcpStats;
//-----------------------------END-------------------------------------------


//----------------------Common for Both RTP and RTCP-------------------------

// RTCP RR table
typedef struct rtcp_rr_table_struct
{
    struct rtcp_rr_table_struct* next;
    struct rtcp_rr_table_struct* prev;
    unsigned int                 reporterSsrc;
    RtcpRrPacket*                rr;
    clocktype                    timeStamp; // Arrival time of this RR
} RtcpRrTable;

// The RTP database contains source-specific
// information needed to make it all work.
typedef struct source_info_struct
{
    struct source_info_struct*  next;
    struct source_info_struct*  prev;
    unsigned int                ssrc;
    char*                       cname;
    char*                       name;
    char*                       email;
    char*                       phone;
    char*                       loc;
    char*                       tool;
    char*                       note;
    char*                       priv;
    RtcpSrPacket*               sr;
    clocktype                   lastSenderReport;
    clocktype                   lastActive;
    // TRUE if this source is a CSRC which we need to advertise SDES for
    int                         shouldAdvertiseSdes;
    int                         sender;
    // TRUE if we have received an RTCP bye from this source
    int                         gotBye;

    unsigned int                baseSeqNum;
    unsigned short              maxSeqNum;
    unsigned int                badSeqNum;
    unsigned int                cycles;
    int                         received;
    int                         recvdPrior;
    int                         expectedPrior;
    int                         probation;
    unsigned int                jitter;
    unsigned int                totalJitter;
    unsigned int                transit;
} RtcpSourceInfo;


// The RtpSession defines an RTP session.
typedef struct rtp_session_str
{

    Address          remoteAddr;
    Address          localAddr;
    unsigned short   remotePort;
    unsigned short   localPort;
    unsigned short   rtcpRemotePort;
    unsigned short   rtcpLocalPort;
    unsigned int     ownSsrc;
    unsigned int     remoteSsrc;
    int              lastAdvertisedCsrc;
    RtcpSourceInfo*  rtpDatabase[RTP_DB_SIZE];
    // Indexed by [hash(reporter)][hash(reportee)]
    RtcpRrTable  rr[RTP_DB_SIZE][RTP_DB_SIZE];
    unsigned char    payloadType;
    int              invalidRtpPktCount;
    int              invalidRtcpPktCount;
    int              probPktDropCount;
    int              byeCountReceived;
    int              csrcCount;
    int              ssrcCount;
    // ssrcCount at the time we last recalculated our RTCP interval
    int              ssrcPrevCount;
    int              senderCount;
    int              intialRtcp;
    // TRUE if we are in the process of sending a BYE packet
    int              sendingBye;
    double           avgRtcpPktSize;
    BOOL             weSentFlag;
    // RTCP bandwidth fraction,in octets per second.
    double           rtcpBandWidth;

    clocktype        lastUpdate;
    clocktype        lastRtpPacketSendTime;
    clocktype        lastRtcpPacketSendTime;
    clocktype        nextRtcpPacketSendTime;
    double           rtcpPacketSendingInterval;
    int              sdesPrimaryCount;
    int              sdesSecondaryCount;
    int              sdesTernaryCount;
    unsigned short   rtpSeqNum;
    unsigned int     rtpPacketCount;
    unsigned int     pktReceived;
    unsigned int     bytesReceived;
    clocktype        totalEndToEndDelay;
    clocktype        rndTripTime;
    clocktype       maxRoundTripTime;
    clocktype       minRoundTripTime;
    unsigned int     rtpByteCount;
    clocktype        sessionStart;
    clocktype        sessionEnd;
    BOOL             activeNow;
    RtcpStats        stats;
    BOOL             gotBye;
    struct rtp_session_str*      prev;
    struct rtp_session_str*      next;
    BOOL             jitter_Buffer_Enable;
    RtpJitterBuffer *jitterBuffer;
    //Added to count no. of RTCP pkts recvd/sent
    unsigned int     rtcppktRecvd, rtcppktSent; 
    BOOL             call_initiator;
    void*            sendFuncPtr;

} RtpSession;
//-----------------------------------END-------------------------------------

//------------------------------RTP Section----------------------------------

//---------------------------------------------------------------------------
// RTP packet structure

// RTP packet: A data packet consisting of the fixed RTP header, a possibly
//             empty list of contributing sources, and the payload data.
//
// The RTP header has the following format:
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                           timestamp                           |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |           synchronization source (SSRC) identifier            |
//   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//   |            contributing source (CSRC) identifiers             |
//   |                             ....                              |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

//--------------------------------------------------------------------------
typedef struct rtp_packet_struct
{
    UInt16   rtpPkt;//v:2;         // packet type
    //unsigned short   p:1;         // padding flag
    //unsigned short   x:1;         // header extension flag
    //unsigned short   cc:4;        // CSRC count
    //unsigned short   m:1;         // marker bit
    //unsigned short   pt:7;        // payload type
    unsigned short   seqNum;      // sequence number
    unsigned int     timeStamp;   // timestamp
    unsigned int     ssrc;        // synchronization source
    // The csrc list, header extension and data follow, but can't
    // be represented in the struct.
} RtpPacket;

//-------------------------------------------------------------------------
// NAME         : RtpPacketSetV()
// PURPOSE      : Set the value of v for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
//            v : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtpPacketSetV( UInt16 *rtpPkt, UInt32 v)
{
    UInt16 v_short = (UInt16)v;

    //masks version within boundry range
    v_short = v_short & maskShort(15, 16);

    //clears the first 2 bits
    *rtpPkt = *rtpPkt & maskShort(3, 16);

    //setting the value of version in rtpPkt
    *rtpPkt = *rtpPkt | LshiftShort(v_short, 2);
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketSetP()
// PURPOSE      : Set the value of p for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
//            p : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtpPacketSetP( UInt16 *rtpPkt, BOOL p)
{
    UInt16 x = (UInt16)p;

    //masks p within boundry range
    x = x & maskShort(16, 16);

    //clears the third bit
    *rtpPkt = *rtpPkt & (~(maskShort(3, 3)));

    //setting the value of p in rtpPkt
    *rtpPkt = *rtpPkt | LshiftShort(x, 3);
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketSetX()
// PURPOSE      : Set the value of x for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
//            x : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtpPacketSetX( UInt16 *rtpPkt, BOOL x)
{
    UInt16 x_short = (UInt16)x;

    //masks x within boundry range
    x_short = x_short & maskShort(16, 16);

    //clears the 4th bit
    *rtpPkt = *rtpPkt & (~(maskShort(4, 4)));

    //setting the value of x in rtpPkt
    *rtpPkt = *rtpPkt | LshiftShort(x_short, 4);
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketSetCc()
// PURPOSE      : Set the value of cc for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
//           cc : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtpPacketSetCc( UInt16 *rtpPkt, UInt32 cc)
{
    UInt16 cc_short = (UInt16)cc;

    //masks cc within boundry range
    cc_short = cc_short & maskShort(13, 16);

    //clears the bit 5-8
    *rtpPkt = *rtpPkt & (~(maskShort(5, 8)));

    //setting the value of cc in rtpPkt
    *rtpPkt = *rtpPkt | LshiftShort(cc_short, 8);
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketSetM()
// PURPOSE      : Set the value of m for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
//            m : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtpPacketSetM( UInt16 *rtpPkt, BOOL m)
{
    UInt16 m_short = (UInt16)m;

    //masks m within boundry range
    m_short = m_short & maskShort(16, 16);

    //clears the 9th bit
    *rtpPkt = *rtpPkt & (~(maskShort(9, 9)));

    //setting the value of m in rtpPkt
    *rtpPkt = *rtpPkt | LshiftShort(m_short, 9);
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketSetPt()
// PURPOSE      : Set the value of pt for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
//           pt : Input value for set operation
// RETURN VALUE : void
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static void RtpPacketSetPt( UInt16 *rtpPkt, unsigned char pt)
{
    UInt16 pt_short = (UInt16)pt;

    //masks pt within boundry range
    pt_short = pt_short & maskShort(10, 16);

    //clears the last 7 bits
    *rtpPkt = *rtpPkt & maskShort(1, 9);

    //setting the value of pt_short in rtpPkt
    *rtpPkt = *rtpPkt | pt_short;
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketGetV()
// PURPOSE      : Returns the value of v for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
// RETURN VALUE : UInt32
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static UInt32 RtpPacketGetV(UInt16 rtpPkt)
{
    UInt16 v = rtpPkt;

    //clears all the bits except first two
    v = v & maskShort(1, 2);

    //right shifts so that last 2 bits represents version
    v = RshiftShort(v, 2);

    return (UInt32)v;
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketGetP()
// PURPOSE      : Returns the value of p for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
// RETURN VALUE : BOOL
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL RtpPacketGetP(UInt16 rtpPkt)
{
    UInt16 p = rtpPkt;

    //clears all the bits except 4th bit
    p = p & maskShort(3, 3);

    //right shifts so that last bit represents p
    p = RshiftShort(p, 3);

    return (BOOL)p;
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketGetX()
// PURPOSE      : Returns the value of x for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
// RETURN VALUE : BOOL
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL RtpPacketGetX(UInt16 rtpPkt)
{
    UInt16 x = rtpPkt;

    //clears all the bits except 4th bit
    x = x & maskShort(4, 4);

    //right shifts so that last bit represents x
    x = RshiftShort(x, 4);

    return (BOOL)x;
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketGetCc()
// PURPOSE      : Returns the value of cc for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
// RETURN VALUE : int
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static int RtpPacketGetCc(UInt16 rtpPkt)
{
    UInt16 cc = rtpPkt;

    //clears all the bits except 5-8
    cc = cc & maskShort(5, 8);

    //right shifts so that last 4 bits represents cc
    cc = RshiftShort(cc, 8);

    return (int)cc;
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketGetM()
// PURPOSE      : Returns the value of m for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
// RETURN VALUE : BOOL
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static BOOL RtpPacketGetM(UInt16 rtpPkt)
{
    UInt16 m = rtpPkt;

    //clears all the bits except 9th bit
    m = m & maskShort(9, 9);

    //right shifts so that last bit represents m
    m = RshiftShort(m, 9);

    return (BOOL)m;
}


//-------------------------------------------------------------------------
// NAME         : RtpPacketGetPt()
// PURPOSE      : Returns the value of pt for RtpPacket
// PARAMETERS   :
//       rtpPkt : The variable containing the value of version,p,x,cc,m and
//                pt
// RETURN VALUE : unsigned char
// ASSUMPTION   : None
//-------------------------------------------------------------------------
static unsigned char RtpPacketGetPt(UInt16 rtpPkt)
{
    UInt16 pt = rtpPkt;

    //clears all the bits except 10-16
    pt = pt & maskShort(10, 16);

    return (unsigned char)pt;
}


typedef struct struct_mapping
{
    unsigned short applicationPort; //application port of the call initiator
    unsigned short rtpPort;// the associated mapped port in RTP Mapping table
    struct_mapping *prev; //pointer to the previous node
    struct_mapping *next; // pointer to the next node
}RtpMappingPort;

// Main data structure
typedef struct
{
    RtpSession*     rtpSession;
    BOOL            sessionOpened;
    unsigned int    totalSession;
    RandomSeed      seed;       /* seed for random number generator */
    BOOL            jitterBufferEnable;
    Int32           maxNoPacketInJitter;
    clocktype       nominalDelayInJitter;
    clocktype       maximumDelayInJitter;
    clocktype       talkspurtDelayInJitter;
    RtpMappingPort* rtpMapping;
    BOOL            rtpStats;
    Int32           rtcpSessionBandwidth;
} RtpData;

// Stats structure returned to application
typedef struct
{

    int              invalidRtpPktCount;
    int              invalidRtcpPktCount;
    int              probPktLostCount;
    unsigned int     rtpPacketCount;
    unsigned int     rtpByteCount;
    unsigned int     pktReceived;
    unsigned int     bytesReceived;
    clocktype        delay;
    unsigned int     avgJitter;
    unsigned int     packetDropped;      // Number of packet dropped due to
                                        // jitter Buffer implementation
    unsigned int     maxConsecutivePacketDropped;      // Maximum Number of
                                        // packet consecutive dropped due to
                                        // jitter Buffer implementation
    clocktype        sessionStart;
    clocktype        sessionEnd;
    BOOL             activeNow;
    unsigned int     ownSsrc;
    unsigned int     remoteSsrc;
    Address          remoteAddr;
    Address          localAddr;
    unsigned short   remotePort;
    unsigned short   localPort;
    unsigned short   rtcpRemotePort;
    unsigned short   rtcpLocalPort;
    unsigned int    rtcpSRSent;
    unsigned int    rtcpSRRecvd;
    unsigned int    rtcpRRSent;
    unsigned int    rtcpRRRecvd;
    unsigned int    rtcpSDESSent;
    unsigned int    rtcpSDESRecvd;
    unsigned int    rtcpBYESent;
    unsigned int    rtcpBYERecvd;
    unsigned int    rtcpAPPSent;
    unsigned int    rtcpAPPRecvd;
    char            src_cname[MAX_STRING_LENGTH];
    char            dest_cname[MAX_STRING_LENGTH];
    clocktype        lastRtpPacketSendTime;
    clocktype        lastRtcpPacketSendTime;
    
    
    
} RtpStats;
//--------------------------------END----------------------------------------

//------------------------- Prototype Declaration ---------------------------
void RtpInit(Node* node,const NodeInput* nodeInput);

void RtcpInit(
    Node*       node,RtpSession* session,char* srcAliasAddr);

void RtpLayer(Node* node, Message* msg);

void RtcpLayer(Node* node, Message* msg);

void RtpFinalize(Node* node);

void RtcpFinalize(Node* node);

void RtpInitiateNewSession(
     Node* node,
     Address        localAddr,
     char* srcAliasAddress,
     Address        remoteAddr,
     clocktype packetizationInterval,
     unsigned short remoteRtpPort,
     unsigned short applicationPayloadType,
     void*          RTPSendFunc);


void RtpTerminateSession(
     Node* node,
     Address        remoteAddr,
     unsigned short initiatorPort);

Message* RtpHandleDataFromApp(
    Node* node,
    Address srcAddr,
    Address destAddr,
    char* payload,
    UInt32 payloadSize,
    UInt8 payloadType,
    UInt32 samplingTime,
    TosType tos,
    UInt16 localPort
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam = NULL
#endif
    );

//------------------------------------------------------------------------
// NAME             : RtpInitializeRtpDataFromConfigFile()
// PURPOSE          : Configure the RtpData structure for Jitter Buffer .
//
// PARAMETERS       :
//          node    : A pointer to Node
//        nodeInput : NodeInput
//          rtpData : A Pointer to rtpData
// ASSUMPTIONS      : None
//
// RETURN           : Nothing
//------------------------------------------------------------------------
void RtpInitializeRtpDataFromConfigFile(Node* node,
                                        RtpData* rtpData,
                                        const NodeInput* nodeInput);

//-------------------------------------------------------------------------
// NAME         : RtpIsJitterBufferEnabled()
//
// PURPOSE      : Check Whether it is the first Packet.
//                  Calculate Maxdelay in Jitter Buffer. Insert
//                  RTP Packet in Jitter Buffer.
//
// PARAMETERS   :
//        node  : A pointer to NODE type.
//      session : A pointer to RtpSession.
//    rtpHeader : A Pointer to RtpPacket.
//          msg : A Pointyer to Message.
// ASSUMPTIONS  : None.
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void RtpIsJitterBufferEnabled(Node* node,
                              RtpSession* session,
                              RtpPacket* rtpHeader,
                              Message* msg);

//-------------------------------------------------------------------------
// NAME                 : RtpIsFirstPacketInJitterBuffer()
//
// PURPOSE              : Create Message for Nominal Delay
//                         Create Message for TalkSput Dely
//
// PARAMETERS           :
//        node          : A pointer to NODE type.
// sessionJitterBuffer  : A pointer to RtpJitterBuffer.
//    rtpHeader         : A Pointer to RtpPacket.
//       msg            : A pointer to Message.
//
// ASSUMPTIONS  : None.
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void RtpIsFirstPacketInJitterBuffer(Node* node,
                                    RtpJitterBuffer* sessionJitterBuffer,
                                    RtpPacket* rtpHeader,
                                    Message* msg);

//-------------------------------------------------------------------------
// NAME             : RtpOnNominalDelayExpired()
//
// PURPOSE          : Send next Sequence Message From Jitter buffer to VOIP
//
// PARAMETERS       :
//        node      : A pointer to NODE type.
//     session      : A pointer to RtpSession.
//      msg         : A Pointer to Message.
//
// ASSUMPTIONS  : None.
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void RtpOnNominalDelayExpired(Node* node,
                              RtpSession* session,
                              Message* msg);

//-------------------------------------------------------------------------
// NAME             : RtpOnTalkSpurtTimerExpired()
//
// PURPOSE          : Update Maximum delay in jitter Buffer
//
// PARAMETERS       :
//        node      : A pointer to NODE type.
//     session      : A pointer to RtpSession.
//      msg         : A Pointer to Message.
//
// ASSUMPTIONS  : None.
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void RtpOnTalkSpurtTimerExpired(Node* node,
                              RtpSession* session,
                              Message* msg);

//------------------------------------------------------------------------
// NAME             : RtpPacketInsertInJitterBuffer()
// PURPOSE          : Insert the packet into Jitter Buffer
// RETURNS          : Nothing
// PARAMETRS        :
//             node : The node
//          session : a Pointer to RtpSession Type
//     jitterBuffer : Jitter buffer associated with session
//              msg : Message
// ASSUMPTION       : None
//------------------------------------------------------------------------

void RtpPacketInsertInJitterBuffer(Node* node,
                                   RtpSession* session,
                                   RtpJitterBuffer* jitterBuffer,
                                   Message* msg);

//------------------------------------------------------------------------
// NAME             : RtpGetLowestSequenceNoPacketFromJitterBufferList()
// PURPOSE          : Get Lowest Sequence Number Packet From
//                  :       Jitter BufferList
// RETURNS          : ListItem Pointer
// PARAMETRS        :
//             node : the node
//     jitterBuffer : Jitter buffer associated with session
// lowestSequenceNo : Lowest Sequence No
// ASSUMPTION       : None
//------------------------------------------------------------------------

ListItem* RtpGetLowestSequenceNoPacketFromJitterBufferList(Node* node,
                                         RtpJitterBuffer* jitterBuffer,
                                         unsigned short& lowestSequenceNo);

//------------------------------------------------------------------------
// NAME             : RtpSendPacketFromJitterBuffer()
// PURPOSE          : Send the Packet From JitterBuffer to VOIP according
//                  :        to given Sequence Number
// RETURNS          : BOOL
//                  :   If send packet is Voice over then return TRUE
//                      else FALSE
// PARAMETRS        :
//             node : the node
//          session : a Pointer to RtpSession Type
//  packetWithLowestSequenceNo : A Pointer to ListItem with Lowest
//                                  Sequence Number Packet
// ASSUMPTION       : None
//-------------------------------------------------------------------------

BOOL RtpSendPacketFromJitterBuffer(Node* node,
                                   RtpSession* session,
                                   ListItem* packetWithLowestSequenceNo);

//-------------------------------------------------------------------------
// NAME         : RtcpGetPacketFromJitterBufferListAndSendIt()
// PURPOSE      : Get the packet from Jitter Buffer List according
//                  to Sequence Number  and Send it
// RETURNS      : void
// PARAMETERS   :
//         node : the Node
//      session : A Pointer of RtpSession
// ASSUMPTION   : First Packet of the Jitter Buffer List
//                  should be Oldest one  as Timestamp
//-------------------------------------------------------------------------
BOOL RtpGetPacketFromJitterBufferListAndSendIt(Node* node,
                                               RtpSession* session);

//-------------------------------------------------------------------------
// NAME                       : RtpDropMaxDelayPacketAndItsFollower()
// PURPOSE                    : Drop First packet and also Drop packets
//                                from Jitter Buffer having sequencenumber
//                                less than FirstPacketSequenceNo.
// RETURN VALUE               :  Nothing
// Parameters                 :
//             node           : Node
//     jitterBuffer           : Jitter Buffer
// sequenceNoOfFirstPacket : expected seq number
// ASSUMPTIONS      : None
//-------------------------------------------------------------------------

void RtpDropMaxDelayPacketAndItsFollower(
    Node* node,
    RtpJitterBuffer* jitterBuffer,
    unsigned short sequenceNoOfFirstPacket);

//-------------------------------------------------------------------------
// NAME                       : RtpIsVoiceOverMessage()
// PURPOSE                    : Whether this is Voice over message?
// RETURN VALUE               : Boolean
// Parameters                 :
//             node           : Node
//     packetData             : Packet data in jitter buffer
// sequenceNoOfFirstPacket : expected seq number
// ASSUMPTIONS      : None
//-------------------------------------------------------------------------

BOOL RtpIsVoiceOverMessage(Node *node,ListItem* packetData);

//-------------------------------------------------------------------------
// NAME                       : RtpClearJitterBufferOnCloseSession()
// PURPOSE                    : Clear Jitter Buffer on Close Session
// RETURN VALUE               :  Nothing
// Parameters                 :
//             node           : Node
//           session          : A pointer of RtpSession of Remote address
// ASSUMPTIONS      : None
//-------------------------------------------------------------------------
void RtpClearJitterBufferOnCloseSession(Node* node, RtpSession* session);

//--------------------------------------------------------------------------
// NAME         : RtpSetJitterBufferFirstPacketAsTrue()
// PURPOSE      : Set Jitter Buffer First Packet is true
// ASSUMPTION   : none
//      node    : Node
//  remoteAddr  : Remote address of this session
// RETURN VALUE : void
//--------------------------------------------------------------------------

void RtpSetJitterBufferFirstPacketAsTrue(Node* node,
                                         Address        remoteAddr,
                                         unsigned short initiatorPort);

//-------------------------------------------------------------------------
// NAME         : RtpCalculateAdaptiveJitterBufferMaxDelay()
// PURPOSE      : Set Jitter Buffer Maximum Delay dynamically
// ASSUMPTION   : none
//      node    : A pointer to Node
//      session : A pointer to RtpSession
//  arrivalTime : Arrival Time of the packet in Receiving Node
// rtpTimestamp : RTP Time Stamp of the packet in Sending Node
// RETURN VALUE : pointer to the RtpSession struct.
//-------------------------------------------------------------------------

void RtpCalculateAdaptiveJitterBufferMaxDelay(Node* node,
                                             RtpSession* session,
                                             clocktype arrivalTime,
                                             clocktype rtpTimestamp);
void RtpGetSessionControlStats(
    Node* node,
    Address destAddr,
    unsigned short localPort,
    RtpStats *stats);
#endif /* RTP_H */
