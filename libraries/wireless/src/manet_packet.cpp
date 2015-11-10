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

// Note: Implementation according to draft-ietf-manet-aodv-09.txt
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "network_ip.h"
#include "ipv6.h"
#include "manet_packet.h"
#include "routing_dymo.h"
#include "buffer.h"

#include "external_socket.h"

//--------------------------------------------------------------------------
// FUNCTION : ConvertNodeAddressToByteArray
// PURPOSE : Function convert node address(IPV4) to an byte array
// ARGUMENTS : NodeAddress, The node IPv4 address
//             charAddr,
// RETURN : void
//--------------------------------------------------------------------------
static
void ConvertNodeAddressToByteArray(
    NodeAddress addr,
    UInt8* charAddr)
{
    int i;
    for(i = 3; i >= 0; i--)
    {
        charAddr[i] = (UInt8)(addr & 0x000000FF);
        addr = addr >> 8;
    }

}


//--------------------------------------------------------------------------
// FUNCTION : ConvertByteArrayToNodeAddress
// PURPOSE : This function convert byte array to node address
// ARGUMENTS : charAddr,
// RETURN : NodeAddress, whose address in converted
//--------------------------------------------------------------------------
static
NodeAddress ConvertByteArrayToNodeAddress(
    UInt8* charAddr)
{
    NodeAddress addr = 0x00000000;
    int i;
    for(i = 0; i <= 2; i++)
    {
        addr |= (NodeAddress)(charAddr[i] & 0xFF);
        addr = addr << 8;
    }
    addr |= (NodeAddress)(charAddr[i] & 0xFF);
    return addr;

}

//--------------------------------------------------------------------------
// FUNCTION : AddAddressTlvBlock
// PURPOSE : Adds Address and Tlv blocks into manet message from address list
//           and tlv list.
//           Assumption : 1.)All the addresses in addlist will be diff
//                        2.)Prefix length will be in bytes
// ARGUMENTS : addlist , address list
//             prefixlen , prefix lenght (Common bits in an IP address)
//             numAdd , number of address to be added
//             mntmsg , Received manet message
//             tlv , Pointer to type length value
// RETURN : void
//--------------------------------------------------------------------------
void AddAddressTlvBlock(
    Address* addlist,
    UInt8 prefixlen,
    UInt8 numAdd,
    ManetMessage* mntmsg,
    TLV* tlv)

{
   mntmsg->addrtlvblock = (AddTlvBlock* ) MEM_malloc(sizeof(AddTlvBlock));
   memset(mntmsg->addrtlvblock, 0, sizeof(AddTlvBlock));

   //DYMO Draft 09 begin
   mntmsg->addrtlvblock->num_addr = numAdd;
   mntmsg->addrtlvblock->addr_semantics = DYMO_ADDRBLK_SEMANTICS;
   //DYMO Draft 09 end

   mntmsg->addrtlvblock->head_length = prefixlen;

   //Fills Head with the first address in the address list
   if( addlist->networkType == NETWORK_IPV4 )
   {
       mntmsg->addrtlvblock->head.networkType = NETWORK_IPV4;
       mntmsg->addrtlvblock->head.interfaceAddr.ipv4 =
                                                addlist->interfaceAddr.ipv4;
   }
   else
   {
    if( addlist->networkType == NETWORK_IPV6 )
    {
        mntmsg->addrtlvblock->head.networkType = NETWORK_IPV6;
        COPY_ADDR6(addlist->interfaceAddr.ipv6,
                   mntmsg->addrtlvblock->head.interfaceAddr.ipv6);
    }
    else
    {
       char buffer[MAX_STRING_LENGTH];
       sprintf(buffer, "Network Type not supported %d", addlist->networkType);
       ERROR_ReportError(buffer);
    }
   }

    mntmsg->addrtlvblock->mid = addlist;
    mntmsg->addrtlvblock->addTlv.tlv = tlv;

}


//--------------------------------------------------------------------------
// FUNCTION : CreateMessage
// PURPOSE : Create empty MANET packet and adds Message Header into it.
// ARGUMENTS : msgtype ,  Type of message to be created (RREQ,RREP,RERR)
//             msgsemantics ,  Message semantics in the ROuting Message
//             origAddr , Origination node IP address
//             hopcount , message hop count value
//             Ttl , message time to live value
//             msgseqnum , message sequence number
// RETURN : Manet Message type
//--------------------------------------------------------------------------
ManetMessage* CreateMessage(
    ManetMessageType msgtype,
    UInt8 msgsemantics,
    Address* origAddr,
    int hopcount,
    int Ttl,
    UInt16 msgseqnum)
{
    ManetMessage *mntmsg = (ManetMessage *) MEM_malloc(sizeof(ManetMessage));
    if( mntmsg == NULL)
    {
        return 0;
    }
    memset(mntmsg, 0, sizeof(ManetMessage));

    mntmsg->message_info.msg_type = msgtype;
    mntmsg->message_info.msg_semantics = msgsemantics;

    if( !(msgsemantics & MANET_MSG_SEMANTIC_BIT_0) )
    {

        ERROR_Assert(origAddr!=NULL, "Orig Addr is NULL !!");

        if(origAddr->networkType == NETWORK_IPV4)
        {
            mntmsg->message_info.headerinfo.origAddr.networkType =
                                                                NETWORK_IPV4;
            mntmsg->message_info.headerinfo.origAddr.interfaceAddr.ipv4 =
                                                origAddr->interfaceAddr.ipv4;
        }
        else
        {
            mntmsg->message_info.headerinfo.origAddr.networkType =
                                                                NETWORK_IPV6;

            COPY_ADDR6(origAddr->interfaceAddr.ipv6,
                mntmsg->message_info.headerinfo.origAddr.interfaceAddr.ipv6);
        }
        mntmsg->message_info.headerinfo.msgseqId = msgseqnum;
        if( !(msgsemantics & MANET_MSG_SEMANTIC_BIT_1) )
        {
            mntmsg->message_info.headerinfo.ttl = (Int8)Ttl;
            mntmsg->message_info.headerinfo.hop_count = (UInt8)hopcount;
        }
    }
    else
    {
        if( !(msgsemantics & MANET_MSG_SEMANTIC_BIT_1) )
        {
            mntmsg->message_info.headerinfo.ttl = (Int8)Ttl;
            mntmsg->message_info.headerinfo.hop_count = (UInt8)hopcount;
        }
        else
        {
            ERROR_Assert(FALSE, "Wrong Message Semantics Value");
        }
    }
   return mntmsg;
}


//--------------------------------------------------------------------------
// FUNCTION : CreatePacket
// PURPOSE : Creates MANET packet
// ARGUMENTS : node, The node who is creatinf the Packet
//             message, Message to be created
// RETURN : Message
//--------------------------------------------------------------------------
Message* CreatePacket(
    Node* node,
    ManetMessage* message)
{
    BOOL isIPV6 = FALSE;
    int numHostBits = 32;
    int i;
    UInt16 msg_size = 0;
    UInt8 mntpkt[MAX_PACKET_SIZE] = {'\0'};
    UInt8* current = mntpkt;
    ManetMessage* mntmsg = message;
    *current = (UInt8) (mntmsg->message_info.msg_type);
    current++;
    *current = mntmsg->message_info.msg_semantics;
    current++;
    current += sizeof(UInt16);    //Message size left empty,
                                  //will be filled in the end

    NetworkRoutingProtocolType protocolType = ROUTING_PROTOCOL_DYMO;

    if( !(mntmsg->message_info.msg_semantics & MANET_MSG_SEMANTIC_BIT_0) )
    {
        if(mntmsg->message_info.headerinfo.origAddr.networkType ==
            NETWORK_IPV4)
        {

            memcpy(current, &(mntmsg->message_info.headerinfo.origAddr.
                            interfaceAddr.ipv4), sizeof(NodeAddress));
            current += sizeof(NodeAddress);
        }
        else
        {
            memcpy(current,&mntmsg->message_info.headerinfo.origAddr.interfaceAddr.ipv6,sizeof(in6_addr));
            current += sizeof(in6_addr);
            isIPV6 = TRUE;
        }

        memcpy(current, &(mntmsg->message_info.headerinfo.msgseqId),
                sizeof(UInt16));
        current += sizeof(UInt16);

        if( !(mntmsg->message_info.msg_semantics
            & MANET_MSG_SEMANTIC_BIT_1) )
        {
            *current = mntmsg->message_info.headerinfo.ttl;
            current += sizeof(Int8);
            *current = mntmsg->message_info.headerinfo.hop_count;
            current++;
        }
    }
    else
    {
        if( !(mntmsg->message_info.msg_semantics
            & MANET_MSG_SEMANTIC_BIT_1) )
        {
            *current = mntmsg->message_info.headerinfo.ttl;
            current += sizeof(Int8);
            *current = mntmsg->message_info.headerinfo.hop_count;
            current++;
        }
        else
        {
            ERROR_Assert(FALSE, "Wrong Message Semantics Value");
        }
    }

    memcpy(current, &(mntmsg->tlvblock.tlv_block_size), sizeof(UInt16));
    current += sizeof(UInt16);
    AddTlvBlock* tempAddTlvBlock = mntmsg->addrtlvblock;
    while(tempAddTlvBlock != NULL)
    {
        // DYMO Draft 09 begin
        *current = (UInt8) (tempAddTlvBlock->num_addr);
        current++;
        *current = (UInt8) (tempAddTlvBlock->addr_semantics);
        current++;
        // DYMO Draft 09 end

        // include <head-length> and <head> (may)
        if(!(tempAddTlvBlock->addr_semantics & DYMO_ADDRBLK_SEMANTICS_BIT_0))
        {
        *current = (UInt8) (tempAddTlvBlock->head_length);
        current++;
        //If Prefix length is zero, packet will not contain Head
        if(tempAddTlvBlock->head_length != 0)
        {
            if(tempAddTlvBlock->head.networkType == NETWORK_IPV4)
            {
                NodeAddress head4;
                //Calculate num host bits for IPv4
                numHostBits =  numHostBits -
                                    (tempAddTlvBlock->head_length * 8);
                head4 =  tempAddTlvBlock->head.interfaceAddr.ipv4;
                UInt8 charHead4[4];
                ConvertNodeAddressToByteArray(head4, charHead4);
                memcpy(current, charHead4, tempAddTlvBlock->head_length);
                current += tempAddTlvBlock->head_length;
            }
            else
            {
                memcpy(current, tempAddTlvBlock->head.interfaceAddr.ipv6.
                        s6_addr, tempAddTlvBlock->head_length);
                current += tempAddTlvBlock->head_length;
                isIPV6 = TRUE;
            }
        }
        }

        // <tail-length> and tail (may)
        if(!(tempAddTlvBlock->addr_semantics & DYMO_ADDRBLK_SEMANTICS_BIT_1))
        {
            *current = (UInt8) (tempAddTlvBlock->tail_length);
            current++;
            //If Prefix length is zero, packet will not contain Head
            if(!(tempAddTlvBlock->addr_semantics & DYMO_ADDRBLK_SEMANTICS_BIT_2))
            {
                if(tempAddTlvBlock->tail_length != 0)
                {
                    if(tempAddTlvBlock->tail.networkType == NETWORK_IPV4)
                    {
                        NodeAddress tail4;
                        //Calculate num host bits for IPv4
                        numHostBits =  numHostBits -
                                            (tempAddTlvBlock->tail_length * 8);
                        tail4 =  tempAddTlvBlock->tail.interfaceAddr.ipv4;
                        UInt8 charTail4[4];
                        ConvertNodeAddressToByteArray(tail4, charTail4);
                        memcpy(current, charTail4, tempAddTlvBlock->tail_length);
                        current += tempAddTlvBlock->tail_length;
                    }
                    else
                    {
                        memcpy(current, tempAddTlvBlock->tail.interfaceAddr.ipv6.
                                s6_addr, tempAddTlvBlock->tail_length);
                        current += tempAddTlvBlock->tail_length;
                        isIPV6 = TRUE;
                    }
                }
            }
        }

        // <mid>
        if(tempAddTlvBlock->head.networkType == NETWORK_IPV4)
        {
            for(i = 0; i < tempAddTlvBlock->num_addr; i++)
            {
                NodeAddress tail4;
                tail4 = tempAddTlvBlock->mid[i].interfaceAddr.ipv4;
                tail4 = tail4 << (tempAddTlvBlock->head_length * 8);
                UInt8 charTail4[4];
                ConvertNodeAddressToByteArray(tail4, charTail4);
                memcpy(current, charTail4, numHostBits / 8);
                current += (numHostBits / 8);
            }
        }
        else
        {
            isIPV6 = TRUE;
            for(i = 0; i < tempAddTlvBlock->num_addr; i++)
            {
                //Copy 16 - Headlength number of bytes into current pointer
                UInt8* temptail = tempAddTlvBlock->mid[i].
                                    interfaceAddr.ipv6.s6_addr;
                memcpy(current,
                       temptail + tempAddTlvBlock->head_length,
                       16 - tempAddTlvBlock->head_length);

                current += (16 - tempAddTlvBlock->head_length);
            }
        }

        //To Calculate TLV Block size
        UInt16 tlvBlksize = 0;
        UInt8* blkSize = current;
        current += sizeof(UInt16);  //TLV Block size left blank,
                                    //will be filled after tlv end

        TLV* tempTLV1 = tempAddTlvBlock->addTlv.tlv;
        TLV* tempTLV2 = NULL;

        while(tempTLV1 != NULL)
        {
            tempTLV2 = tempTLV1->next;
            *current = (UInt8)tempTLV1->tlvType;
            current++;
            tlvBlksize = tlvBlksize + sizeof(UInt8);

            *current = tempTLV1->tlv_semantics;
            current++;
            tlvBlksize = tlvBlksize + sizeof(UInt8);

            // Manet Packet Format 03
            if(!(tempTLV1->tlv_semantics & DYMO_ADD_TLV_SEMANTICS_BIT_2))
            {
            *current = tempTLV1->index_start;
            current++;
            tlvBlksize = tlvBlksize + sizeof(UInt8);

                if(!(tempTLV1->tlv_semantics & DYMO_ADD_TLV_SEMANTICS_BIT_3))
                {
            *current = tempTLV1->index_stop;
            current++;
            tlvBlksize = tlvBlksize + sizeof(UInt8);
                }
            }

            if(!(tempTLV1->tlv_semantics & DYMO_ADD_TLV_SEMANTICS_BIT_1))
            {
                if(tempTLV1->tlv_semantics & DYMO_ADD_TLV_SEMANTICS_BIT_0)
            {
                memcpy(current,
                       &(tempTLV1->TLV_length),
                       sizeof(UInt16));
                current += sizeof(UInt16);
                tlvBlksize = tlvBlksize + sizeof(UInt16);
                }else
                {
                    memcpy(current,
                           &(tempTLV1->TLV_length),
                           sizeof(UInt8));
                    current += sizeof(UInt8);
                    tlvBlksize = tlvBlksize + sizeof(UInt8);
                }

                ERROR_Assert(tempTLV1->TLV_length,
                            "Manet packet tlv.length equal to zero.");

                memcpy(current,
                       tempTLV1->val,
                       tempTLV1->TLV_length);
                current += tempTLV1->TLV_length;
                tlvBlksize = tlvBlksize + tempTLV1->TLV_length;
            }
            tempTLV1 = tempTLV2;
        }
        memcpy(blkSize, &tlvBlksize, sizeof(UInt16));
        tempAddTlvBlock = tempAddTlvBlock->next;
    }

    msg_size = (UInt16)(current - mntpkt);
    if(msg_size > MAX_PACKET_SIZE)
    {
        ERROR_Assert(FALSE, "MANET pkt size exceeded MAXIMUM PACKET SIZE");
    }
    if((msg_size % 4) != 0) //To make Msg size multiple of 4
    {
        int noOfPaddingBytes = 4 - (msg_size % 4);
        msg_size = msg_size + (UInt16)noOfPaddingBytes;
    }

    current = mntpkt;
    current += 2;
    memcpy(current, &msg_size, sizeof(UInt16)); // fix for solaris


    if(isIPV6)
    {
        protocolType = ROUTING_PROTOCOL_DYMO6;
    }

    Message* msg = MESSAGE_Alloc(node,
                        NETWORK_LAYER,
                        protocolType,
                        MSG_MAC_FromNetwork);

    MESSAGE_PacketAlloc(
        node,
        msg,
        msg_size,
        TRACE_DYMO);

    ManetMessage* packet = (ManetMessage* )MESSAGE_ReturnPacket(msg);
    memcpy(packet, mntpkt, msg_size);
    return msg;
}


//--------------------------------------------------------------------------
// FUNCTION : ParsePacket
// PURPOSE : Printing the different fields of the routing table of Dymo
// ARGUMENTS : node, Node which receive the packet and need to parse
//             msg, Pointer to message data structure (Previous hop info)
//             networkType,Node address network type (IPV4/IPV6)
// RETURN : ManetMessage
//--------------------------------------------------------------------------
ManetMessage* ParsePacket(
    Node* node,
    Message* msg,
    NetworkType networkType)
{
    int numHostBits = 32;
    UInt8 numtail;
    UInt16 tlvblksize;

    ManetMessage* mntmsg = (ManetMessage *) MEM_malloc(sizeof(ManetMessage));
    if( mntmsg == NULL)
    {
        return 0;
    }
    memset(mntmsg, 0, sizeof(ManetMessage));

    UInt8* mntpkt = (UInt8* ) MESSAGE_ReturnPacket(msg);
    mntmsg->message_info.msg_type = (ManetMessageType)((UInt8)(*mntpkt));
    mntpkt++;
    mntmsg->message_info.msg_semantics = (UInt8)(*mntpkt);
    mntpkt++;
    memcpy(&(mntmsg->message_info.msg_size), mntpkt, sizeof(UInt16));
    mntpkt += sizeof(UInt16);
    UInt16 pktlen = 4; //Since msg type,semantics & msg size will always be
                       //there

    if( !(mntmsg->message_info.msg_semantics & MANET_MSG_SEMANTIC_BIT_0) )
    {

        //ERROR_Assert(origAddr!=NULL, "Orig Addr is NULL !!");

        if(networkType == NETWORK_IPV4)
        {
            mntmsg->message_info.headerinfo.origAddr.networkType =
                                                                NETWORK_IPV4;

            memcpy(&(mntmsg->message_info.headerinfo.origAddr.
                    interfaceAddr.ipv4), mntpkt, sizeof(NodeAddress));

            mntpkt += sizeof(NodeAddress);
            pktlen += 4;
        }
        else
        {
            mntmsg->message_info.headerinfo.origAddr.networkType =
                                                                NETWORK_IPV6;

            memcpy(
              &(mntmsg->message_info.headerinfo.origAddr.interfaceAddr.ipv6),
              mntpkt,
              sizeof(in6_addr)
              );

            mntpkt += sizeof(in6_addr);
            pktlen += 16;
        }
        memcpy(&(mntmsg->message_info.headerinfo.msgseqId), mntpkt,
                sizeof(UInt16));
        mntpkt += sizeof(UInt16);
        pktlen += 2;

        if( !(mntmsg->message_info.msg_semantics & MANET_MSG_SEMANTIC_BIT_1) )
        {
            mntmsg->message_info.headerinfo.ttl = (Int8)(*mntpkt);
            mntpkt += sizeof(Int8);
            mntmsg->message_info.headerinfo.hop_count = (UInt8)(*mntpkt);
            mntpkt++;
            pktlen += 2;
        }
    }
    else
    {
        if( !(mntmsg->message_info.msg_semantics & MANET_MSG_SEMANTIC_BIT_1) )
        {
            mntmsg->message_info.headerinfo.ttl = (Int8)(*mntpkt);
            mntpkt += sizeof(Int8);
            mntmsg->message_info.headerinfo.hop_count = (UInt8)(*mntpkt);
            mntpkt++;
            pktlen += 2;
        }
        else
        {
            ERROR_Assert(FALSE, "Wrong Message Semantics Value");
        }
    }

    //Copy Msg TLV Length into mntmsg
    memcpy(&(mntmsg->tlvblock.tlv_block_size), mntpkt, sizeof(UInt16));
    mntpkt += sizeof(UInt16);
    pktlen += 2;

    //Chking if an address-tlv block is present or not
    if( (mntmsg->message_info.msg_size - pktlen) > 3 )
    {
        mntmsg->addrtlvblock = (AddTlvBlock* ) MEM_malloc(sizeof(AddTlvBlock));
        memset(mntmsg->addrtlvblock, 0, sizeof(AddTlvBlock));

        // DYMO Draft 09 begins
        mntmsg->addrtlvblock->num_addr = (UInt8)(*mntpkt);
        mntpkt++;
        pktlen++;

        mntmsg->addrtlvblock->addr_semantics = (UInt8)(*mntpkt);
        mntpkt++;
        pktlen++;
        // DYMO Draft 09 ends

        //Use #define instead of 16
        UInt8 aadrstr[16] = {'\0'};

        // <head-length> and <head> (may)
        if(!(mntmsg->addrtlvblock->addr_semantics & DYMO_ADDRBLK_SEMANTICS_BIT_0))
        {
        mntmsg->addrtlvblock->head_length = (UInt8)(*mntpkt);
        mntpkt++;
        pktlen++;

        if(mntmsg->addrtlvblock->head_length != 0)
        {
            if( networkType == NETWORK_IPV4 )
            {
                NodeAddress addr4;
                numHostBits =  numHostBits -
                                    (mntmsg->addrtlvblock->head_length * 8);
                mntmsg->addrtlvblock->head.networkType = NETWORK_IPV4;
                memcpy(aadrstr, mntpkt, mntmsg->addrtlvblock->head_length);
                addr4 = ConvertByteArrayToNodeAddress(aadrstr);
                mntmsg->addrtlvblock->head.interfaceAddr.ipv4 = addr4;
                mntpkt += mntmsg->addrtlvblock->head_length;
                pktlen = pktlen + (UInt16)mntmsg->addrtlvblock->head_length;
            }
            else
            {
                if( networkType == NETWORK_IPV6 )
                {
                    mntmsg->addrtlvblock->head.networkType = NETWORK_IPV6;
                    //Copy HeadLength number of bytes into ipv6 address
                    memcpy(mntmsg->addrtlvblock->head.interfaceAddr.ipv6.
                            s6_addr, mntpkt,
                            mntmsg->addrtlvblock->head_length);

                    mntpkt += mntmsg->addrtlvblock->head_length;
                    pktlen =
                        pktlen + (UInt16)mntmsg->addrtlvblock->head_length;
                }
                else
                {
                    char buffer[MAX_STRING_LENGTH];
                    sprintf(buffer, "Network Type not supported %d",
                        networkType);
                    ERROR_ReportError(buffer);
                }
            }
        }else {
            // fix in the case if head_length == 0, the network type
            // will be used in createpacket for relayPacket
            // createPacket only considers head.networkType
            mntmsg->addrtlvblock->head.networkType = networkType;
        }
        }

        UInt8 badrstr[16] = {'\0'};
        // <tail-length> and <tail> (may)
        if(!(mntmsg->addrtlvblock->addr_semantics & DYMO_ADDRBLK_SEMANTICS_BIT_1))
        {
            mntmsg->addrtlvblock->tail_length = (UInt8)(*mntpkt);
            mntpkt++;
            pktlen++;

            if(!(mntmsg->addrtlvblock->addr_semantics & DYMO_ADDRBLK_SEMANTICS_BIT_2))
            {
                if(mntmsg->addrtlvblock->tail_length != 0)
                {
                    if(networkType == NETWORK_IPV4 )
                    {
                        NodeAddress addr4;
                        numHostBits =  numHostBits -
                                            (mntmsg->addrtlvblock->tail_length * 8);
                        mntmsg->addrtlvblock->tail.networkType = NETWORK_IPV4;
                        memcpy(badrstr, mntpkt, mntmsg->addrtlvblock->tail_length);
                        addr4 = ConvertByteArrayToNodeAddress(badrstr);
                        mntmsg->addrtlvblock->tail.interfaceAddr.ipv4 = addr4;
                        mntpkt += mntmsg->addrtlvblock->tail_length;
                        pktlen = pktlen +
                            (UInt16)mntmsg->addrtlvblock->tail_length;
                    }
                    else
                    {
                        if( networkType == NETWORK_IPV6 )
                        {
                            mntmsg->addrtlvblock->tail.networkType = NETWORK_IPV6;
                            //Copy HeadLength number of bytes into ipv6 address
                            memcpy(mntmsg->addrtlvblock->tail.interfaceAddr.ipv6.
                                    s6_addr, mntpkt,
                                    mntmsg->addrtlvblock->tail_length);

                            mntpkt += mntmsg->addrtlvblock->tail_length;
                            pktlen = pktlen +
                                (UInt16)mntmsg->addrtlvblock->tail_length;
                        }
                        else
                        {
                            char buffer[MAX_STRING_LENGTH];
                            sprintf(buffer, "Network Type not supported %d",
                                networkType);
                            ERROR_ReportError(buffer);
                        }
                    }
                }else {
                    // fix in the case if head_length == 0, the network type
                    // will be used in createpacket for relayPacket
                    // createPacket only considers head.networkType
                    mntmsg->addrtlvblock->tail.networkType = networkType;
                }
            }
        }

        // <mid>*
        numtail = 0;
        // Will not allocate memory if num tails is zero
        if(mntmsg->addrtlvblock->num_addr != 0)
        {
            mntmsg->addrtlvblock->mid = (Address* )
                MEM_malloc(sizeof(Address) * mntmsg->addrtlvblock->num_addr);
            memset(mntmsg->addrtlvblock->mid, 0,
                sizeof(Address) * mntmsg->addrtlvblock->num_addr);
        }
        if(networkType == NETWORK_IPV4)
        {
            while(numtail < mntmsg->addrtlvblock->num_addr)
            {
                mntmsg->addrtlvblock->mid[numtail].networkType =
                                                            NETWORK_IPV4;
                NodeAddress tail4;
                UInt8 temptail[16] = {'\0'};
                if(mntmsg->addrtlvblock->head_length != 0)
                {
                    memcpy(temptail, aadrstr,
                            mntmsg->addrtlvblock->head_length);
                }
                memcpy(temptail + mntmsg->addrtlvblock->head_length, mntpkt,
                        numHostBits / 8);

                if(mntmsg->addrtlvblock->tail_length != 0)
                {
                    memcpy(temptail + mntmsg->addrtlvblock->head_length+numHostBits / 8,
                        badrstr, mntmsg->addrtlvblock->tail_length);
                }
                tail4 = ConvertByteArrayToNodeAddress(temptail);
                mntmsg->addrtlvblock->mid[numtail].interfaceAddr.ipv4 =
                                                                       tail4;
                if( (numtail == 0)
                    && (mntmsg->addrtlvblock->head_length != 0) )
                {
                    mntmsg->addrtlvblock->head.interfaceAddr.ipv4 = mntmsg->
                            addrtlvblock->mid[numtail].interfaceAddr.ipv4;
                }

                mntpkt += (numHostBits / 8);
                pktlen += ((UInt16) numHostBits / 8);
                numtail++;
            }
        }
        else
        {
            while(numtail < mntmsg->addrtlvblock->num_addr)
            {
                mntmsg->addrtlvblock->mid[numtail].networkType =
                                                            NETWORK_IPV6;
                UInt8* temptail = mntmsg->addrtlvblock->mid[numtail].
                                    interfaceAddr.ipv6.s6_addr;
                if(mntmsg->addrtlvblock->head_length != 0)
                {
                    memcpy(temptail, &(mntmsg->addrtlvblock->head.interfaceAddr.ipv6),
                            mntmsg->addrtlvblock->head_length);
                }
                UInt8 common_length = mntmsg->addrtlvblock->head_length
                    + mntmsg->addrtlvblock->tail_length;
                memcpy(temptail + mntmsg->addrtlvblock->head_length, mntpkt,
                        16 - common_length);
                if(mntmsg->addrtlvblock->tail_length != 0)
                {
                    memcpy(temptail + 16 - mntmsg->addrtlvblock->tail_length,
                        &(mntmsg->addrtlvblock->tail.interfaceAddr.ipv6),
                        mntmsg->addrtlvblock->tail_length);
                }
                mntpkt += (16 - common_length);
                pktlen += (16 - common_length);
                numtail++;
            }

        }
        memcpy(&(mntmsg->addrtlvblock->addTlv.tlv_block_size), mntpkt,
                sizeof(UInt16));
        mntpkt += sizeof(UInt16);
        pktlen += 2;
        if(mntmsg->addrtlvblock->addTlv.tlv_block_size != 0)
        {
            tlvblksize = 0;
            mntmsg->addrtlvblock->addTlv.tlv = NULL;

            while(tlvblksize < mntmsg->addrtlvblock->addTlv.tlv_block_size)
            {
                TLV* temp = (TLV* ) MEM_malloc(sizeof(TLV));
                memset(temp, 0, sizeof(TLV));
                temp->tlvType = (ManetAddressTlvType)((UInt8)(*mntpkt));
                mntpkt++;
                pktlen++;
                tlvblksize++;

                temp->tlv_semantics =
                    (UInt8)((ManetAddressTlvType)(*mntpkt));
                mntpkt++;
                pktlen++;
                tlvblksize++;

                // DYMO Draft 09 Manet Packet/Message Format 03
                if(!(temp->tlv_semantics & DYMO_ADD_TLV_SEMANTICS_BIT_2))
                {
                temp->index_start = (UInt8)((ManetAddressTlvType)(*mntpkt));
                mntpkt++;
                pktlen++;
                tlvblksize++;

                    if(!(temp->tlv_semantics & DYMO_ADD_TLV_SEMANTICS_BIT_3))
                    {
                temp->index_stop = (UInt8)((ManetAddressTlvType)(*mntpkt));
                mntpkt++;
                pktlen++;
                tlvblksize++;
                    }else {
                        temp->index_stop = temp->index_start;
                    }
                }else {
                    temp->index_start = 0;
                    temp->index_stop = mntmsg->addrtlvblock->num_addr-1;
                }

                switch (temp->tlvType)
                {
                    // Assuming TLV length can never be zero

                    case PREFIX :
                    case DYMO_SEQNUM :
                    case HOP_COUNT :
                    {
                        if(!(temp->tlv_semantics & DYMO_ADD_TLV_SEMANTICS_BIT_1))
                        {
                            if(temp->tlv_semantics & DYMO_ADD_TLV_SEMANTICS_BIT_0)
                            {
                        memcpy(&(temp->TLV_length), mntpkt, sizeof(UInt16));
                        mntpkt = mntpkt + sizeof(UInt16);
                        pktlen += 2;
                        tlvblksize += 2;
                            }else
                            {
                                memcpy(&(temp->TLV_length), mntpkt, sizeof(UInt8));
                                mntpkt = mntpkt + sizeof(UInt8);
                                pktlen += 1;
                                tlvblksize += 1;
                            }

                            ERROR_Assert(temp->TLV_length,
                                "Manet packet tlv.length equal to zero.");
                        temp->val = (UInt8* )
                            MEM_malloc(temp->TLV_length);
                        memcpy(temp->val, mntpkt, temp->TLV_length);
                        mntpkt += temp->TLV_length;
                        pktlen = pktlen + temp->TLV_length;
                        tlvblksize = tlvblksize + temp->TLV_length;
                    }

                        break;
                    }
                    case GATEWAY :
                    {
                        //Need to perform any action ??

                        break;
                    }
                    default:
                    {
                        //Shouldn't get here.
                        ERROR_Assert(FALSE, "Invalid TLV Type\n");
                    }
                }
                if(mntmsg->addrtlvblock->addTlv.tlv == NULL)
                {
                    mntmsg->addrtlvblock->addTlv.tlv = temp;
                }
                else
                {
                    //Adding TLVs in front of LL
                    temp->next = mntmsg->addrtlvblock->addTlv.tlv;
                    mntmsg->addrtlvblock->addTlv.tlv = temp;

                }
            }
        }
    }
    return mntmsg;
}


//--------------------------------------------------------------------------
// FUNCTION : Packet_Free
// PURPOSE : Frees memory allocated to a MANET packet
// ARGUMENTS : node, pointer to node data structure
//             mntmsg, Received manet message
// RETURN : void
//--------------------------------------------------------------------------
void Packet_Free(
    Node* node,
    ManetMessage* mntmsg)
{
    if(mntmsg != NULL){
        TLV* tempTLV1 = mntmsg->tlvblock.tlv;
        TLV* tempTLV2 = NULL;
        while(tempTLV1 != NULL){
             tempTLV2 = tempTLV1->next;
             if(tempTLV1->val != NULL){
                MEM_free(tempTLV1->val);
                tempTLV1->val = NULL;
             }
             MEM_free(tempTLV1);
             tempTLV1 = tempTLV2;
        }// end of while tlv
        tempTLV1 = tempTLV2 = NULL;
        AddTlvBlock* tempAddTlvBlock1 = mntmsg->addrtlvblock;
        AddTlvBlock* tempAddTlvBlock2 = NULL;
        while(tempAddTlvBlock1 != NULL){
            tempAddTlvBlock2 = tempAddTlvBlock1->next;
            if( tempAddTlvBlock1->mid != NULL){
                MEM_free(tempAddTlvBlock1->mid);
                tempAddTlvBlock1->mid = NULL;
            }
            tempTLV1 = tempAddTlvBlock1->addTlv.tlv;
            while(tempTLV1 != NULL){
                tempTLV2 = tempTLV1->next;
                if(tempTLV1->val != NULL){
                   MEM_free(tempTLV1->val);
                   tempTLV1->val = NULL;
                }
                MEM_free(tempTLV1);
                tempTLV1 = tempTLV2;
            }
            tempTLV1 = tempTLV2 = NULL;
            MEM_free(tempAddTlvBlock1);
            tempAddTlvBlock1 = tempAddTlvBlock2;
        }// end of while addTlvBlock
        tempAddTlvBlock1 = tempAddTlvBlock2 = NULL;
        MEM_free(mntmsg);
        mntmsg = NULL;
    }
}
