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

#ifndef MANET_H

#define MANET_H

#define MANET_MSG_SEMANTIC_BIT_0             0x01
#define MANET_MSG_SEMANTIC_BIT_1             0x02

//Assigned message TLV Types
#define Fragmentation     0        //Specifies behavior in case of content
                                //fragmentation
#define Content_Seq_num  1        // A sequence number. associated with the
                                //content of the mesage

#define MAX_PACKET_SIZE 2048

//Assigned address block TLV Types
#define PREFIXLENGTH     0        //Indicates that  associated address are
                                //network addresses,with given prefix length


//Assigned manet message Type
typedef enum{

        RESERVED=0,        //signals that the following 24 bits are a packet
                        //header ,rather than a message header
   OLSRV1HELLO = 1,        //Reserved for compatibility with OLSRv1
      OLSRV1TC = 2,        //Reserved for compatibility with OLSRv1
     OLSRV1MID = 3,        //Reserved for compatibility with OLSRv1
     OLSRV1HNA = 4,        //Reserved for compatibility with OLSRv1
 ROUTE_REQUEST = 10, //defined for DYMO 
   ROUTE_REPLY = 11,
   ROUTE_ERROR = 12
}ManetMessageType;

//Assigned manet address tlv types
typedef enum{
        PREFIX = 0,
  DYMO_SEQNUM  = 10, //dymo sequence number tlv type
    HOP_COUNT  = 11,
    MAX_AGE = 12,
       GATEWAY = 22,
    ORIGINATOR = 23,
        TARGET = 24
}ManetAddressTlvType;

//-------------------------------------------------------------------------
// STRUCT         ::    MsgHeaderInfo
// DESCRIPTION    ::    message_info-information field structure
//-------------------------------------------------------------------------
typedef struct {

    Address       origAddr;
    Int8          ttl;                //Time to live value of a packet
                                        //between intermediating node
    UInt8         hop_count;            //Hop count of a packet,increment
    UInt16        msgseqId;

}MsgHeaderInfo;

//-------------------------------------------------------------------------
// STRUCT         ::    MsgHeader
// DESCRIPTION    ::    manet message header structure
//-------------------------------------------------------------------------
typedef struct {
    ManetMessageType    msg_type; //Message type,whether it is RREQ, RREP
                                  //or RERR
    UInt8                msg_semantics;  //8 bit field,which tell us message
                                        //header information
    UInt16                msg_size;        //size of the message

    MsgHeaderInfo        headerinfo;

}MsgHeader;

//-------------------------------------------------------------------------
// STRUCT         ::    TLV
// DESCRIPTION    ::    type length value structure
//-------------------------------------------------------------------------
typedef struct tlv {
    ManetAddressTlvType      tlvType;
    UInt8                    tlv_semantics; //Not used right now
    UInt8                    index_start;
    UInt8                    index_stop;
    UInt16                   TLV_length;
    UInt8*                   val;
    struct tlv*              next;
}TLV;

//-------------------------------------------------------------------------
// STRUCT         ::    TlvBlock
// DESCRIPTION    ::    type lenght value block structure
//-------------------------------------------------------------------------
typedef struct {
    UInt16 tlv_block_size;   //it contain total length in octet of the
                             //immediately following tlv, if no tlv follows
                             // this field contain zero.

    TLV* tlv;                 // A TLV is a carrier of information,
                             //relative to a message or to addresses
                             //in an address block.
}TlvBlock;

//-------------------------------------------------------------------------
// STRUCT         ::    AddBlock
// DESCRIPTION    ::    address block structure
//-------------------------------------------------------------------------
typedef struct addtlvblock {
    
    UInt8 num_addr; // DYMO Draft 09 
            // containing the number of addresses represented
            // in the address block, which MUST NOT be zero.

    UInt8 addr_semantics; // DYMO Draft 09
            // specifying the interpretation of the remainder
            // of the address block

    UInt8 head_length;        // Is the number of "common leftmost
                            //octets" in a set of addresses, where
                            //<=head-length<=the length of address
                            //in octets
    Address  head;            // It is the longestsequence of leftmost
                            //octets which the addresses in the
                            //address block have in common.
    UInt8 tail_length;      // if present, which contains the total length (in
                            // octets) of the tail of all of the addresses.
    Address tail;
    Address* mid;
    TlvBlock addTlv;
    struct addtlvblock* next;
}AddTlvBlock;

//-------------------------------------------------------------------------
// STRUCT         ::    ManetMessage
// DESCRIPTION    ::    manet common message(RREQ/RREP/RERR) Routing Message
//                        structure
//-------------------------------------------------------------------------

typedef struct manet_msg {
    MsgHeader      message_info;
    TlvBlock       tlvblock;
    AddTlvBlock*   addrtlvblock;
}ManetMessage;


//-------------------------------------------------------------------------
// STRUCT         ::    FragementTlv
// DESCRIPTION    ::    structure for fragemented TLV
//-------------------------------------------------------------------------
typedef struct{
    UInt8 num_of_fragment ;
    UInt8 fragment_num;
    UInt8 bit_vector;
}FragementTlv;

ManetMessage* CreateMessage(
    ManetMessageType msgtype,
    UInt8 msgsemantics,
    Address* origAddr,
    int hopcount,
    int Ttl,
    UInt16 msgseqnum);

void AddAddressTlvBlock(
    Address* addlist,
    UInt8 prefixlen,
    UInt8 numAdd,
    ManetMessage* mntmsg,
    TLV* tlv);

Message* CreatePacket(
    Node* node,
    ManetMessage* message);

ManetMessage* ParsePacket(
    Node* node,
    Message* msg,
    NetworkType networkType);

void Packet_Free(
    Node* node,
    ManetMessage* mntmsg);

#endif /* MANET_H */

