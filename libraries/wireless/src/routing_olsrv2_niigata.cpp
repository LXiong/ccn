/*
 *
 * Copyright (c) 2006, Graduate School of Niigata University,
 *                                         Ad hoc Network Lab.
 * Developer:
 *  Yasunori Owada  [yowada@net.ie.niigata-u.ac.jp],
 *  Kenta Tsuchida  [ktsuchi@net.ie.niigata-u.ac.jp],
 *  Taka Maeno      [tmaeno@net.ie.niigata-u.ac.jp],
 *  Hiroei Imai     [imai@ie.niigata-u.ac.jp].
 * Contributor:
 *  Keita Yamaguchi [kyama@net.ie.niigata-u.ac.jp],
 *  Yuichi Murakami [ymura@net.ie.niigata-u.ac.jp],
 *  Hiraku Okada    [hiraku@ie.niigata-u.ac.jp].
 *
 * This software is available with usual "research" terms
 * with the aim of retain credits of the software.
 * Permission to use, copy, modify and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies,
 * and the name of NIIGATA, or any contributor not be used in advertising
 * or publicity pertaining to this material without the prior explicit
 * permission. The software is provided "as is" without any
 * warranties, support or liabilities of any kind.
 * This product includes software developed by the University of
 * California, Berkeley and its contributors protected by copyrights.
 */
//qualnet
#include "api.h"
#include "app_util.h"
#include "network_ip.h"
#include "ipv6.h"
#include "ipv6_route.h"
#include "random.h"

// Fix for vc9 compilation
#if (_MSC_VER >= 1500) //vc9
#ifdef IPPROTO_IP
#undef IPPROTO_IP
#endif
#ifdef IPPROTO_ICMP
#undef IPPROTO_ICMP
#endif
#ifdef IPPROTO_IGMP
#undef IPPROTO_IGMP
#endif
#ifdef IPPROTO_TCP
#undef IPPROTO_TCP
#endif
#ifdef IPPROTO_UDP
#undef IPPROTO_UDP
#endif
#endif

//Fix for Solaris compilation
#if !defined (WIN32) && !defined (_WIN32) && !defined (_WIN64)
    #ifndef _NETINET_IN_H
    #define _NETINET_IN_H
    #endif
#endif

//olsrv2_niigata
#include "proc_rcv_msg_set.h"
#include "proc_proc_set.h"
#include "proc_relay_set.h"
#include "proc_forward_set.h"
#include "proc_link_set.h"
#include "proc_2neigh_set.h"
#include "proc_topology_set.h"
#include "proc_adv_neigh_set.h"
#include "proc_routing_set.h"
#include "proc_mpr_set.h"
#include "proc_mpr_selector_set.h"
#include "proc_attach_network_set.h"
#include "proc_assn_history_set.h"
#include "proc_neighbor_address_association_set.h"
#include "generate_msg.h"
#include "parse_msg.h"

#include "olsr_types.h"
#include "olsr_debug.h"
#include "olsr_conf.h"
#include "olsr_util.h"
#include "olsr_list.h"
#include "olsr.h"
#include "pktbuf.h"
#include "proc_symmetric_neighbor_set.h"

#include "olsr_util_inline.h"
#include "routing_olsrv2_niigata.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#define MAXMESSAGESIZE      1500



#define DEBUG_OUTPUT 0
//#define DEBUG_OUTPUT 1

int random(struct olsrv2 *olsr)
{

    return (RANDOM_nrand(olsr->seed) % 32767);
}


static void
RoutingOLSRv2_Niigata_DumpPacket(char *packet, olsr_u16_t size)
{
  int i;
  char *p = NULL;
  if (DEBUG_OLSRV2){
      olsr_printf("Dump packet of size = %d\n",size);
  }
  p = packet;
  for (i = 0 ; i < size ; i++){
    if (i%4==0)
      olsr_printf("\n");
    olsr_printf("%3d\t",(olsr_u8_t)*p);
    p++;
  }
}

/* Shift to olsr_conf.cpp for MAC mismatch
MACHINE_TYPE GetMachineType()
{
    union type{
        char a;
        int b;
    }m_type;

    m_type.b = 0xff;

    if (m_type.a == 0)
    {
    return B_ENDIAN;
    }
    else
    {
    return L_ENDIAN;
    }
}*/

static void
RoutingOLSRv2_Niigata_RotateShiftByte(olsr_u8_t *data, olsr_u16_t size)
{
    if (GetMachineType()== B_ENDIAN)
    {
    return;
    }

    olsr_u8_t *buf = NULL;

    assert(size > 0);
    buf = (olsr_u8_t *)MEM_malloc(size);
    memset(buf, 0, size);
    memcpy(buf, data, size);

    for (int i=0; i<size; i++)
    {
    data[i] = buf[size-i-1];
    }
    MEM_free(buf);
}

static Address*
RoutingOLSRv2_Niigata_GetQualnetAddress(struct olsrv2 *olsr, union olsr_ip_addr olsrv2_niigata)
{
    olsrd_config* olsr_cnf = olsr->olsr_cnf;


    Address* qualnet_addr = (Address*)MEM_malloc(sizeof(Address));
    memset(qualnet_addr, 0, sizeof(Address));


    if (olsr_cnf->ip_version == AF_INET){
    qualnet_addr->networkType = NETWORK_IPV4;
    memcpy(&qualnet_addr->interfaceAddr.ipv4,
           &olsrv2_niigata.v4, olsr_cnf->ipsize);
    RoutingOLSRv2_Niigata_RotateShiftByte
        ((olsr_u8_t *)&qualnet_addr->interfaceAddr.ipv4,
        (olsr_u16_t)olsr_cnf->ipsize);
    }else{
    qualnet_addr->networkType = NETWORK_IPV6;
    memcpy(&qualnet_addr->interfaceAddr.ipv6.s6_addr,
           olsrv2_niigata.v6.s6_addr, olsr_cnf->ipsize);
    }

    return qualnet_addr;
}

static union olsr_ip_addr*
RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(struct olsrv2 *olsr, Address qualnet_addr)
{
    olsrd_config* olsr_cnf = olsr->olsr_cnf;

    union olsr_ip_addr* olsrv2_niigata =
    (olsr_ip_addr*)MEM_malloc(sizeof(union olsr_ip_addr));
    memset(olsrv2_niigata, 0, sizeof(union olsr_ip_addr));

    if (qualnet_addr.networkType == NETWORK_IPV4){
    memcpy(&olsrv2_niigata->v4,
           &qualnet_addr.interfaceAddr.ipv4,
           olsr_cnf->ipsize);
    RoutingOLSRv2_Niigata_RotateShiftByte
      ((olsr_u8_t *)&olsrv2_niigata->v4, (olsr_u16_t) olsr_cnf->ipsize);

    }else{
    memcpy(&olsrv2_niigata->v6.s6_addr_8,
           &qualnet_addr.interfaceAddr.ipv6.s6_addr, olsr_cnf->ipsize);
    }

    return olsrv2_niigata;
}

static
olsr_u8_t * //new_msg
RoutingOLSRv2_Niigata_AddPacketHeader(Node *node,
                      olsr_u8_t *msg,
                      olsr_u32_t msg_size,
                      olsr_u32_t *new_msg_size)
{
  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

  olsr_u8_t *new_msg = NULL;
  struct packet_header *p_header = NULL;

  *new_msg_size = msg_size + sizeof(struct packet_header);
  new_msg = (olsr_u8_t *)MEM_malloc(*new_msg_size);

  p_header = (struct packet_header *)new_msg;
  p_header->packet_seq_num = get_packet_seqno(olsr);
  p_header->zero = 0;
  p_header->packet_semantics =0;

  RoutingOLSRv2_Niigata_RotateShiftByte((olsr_u8_t *)&p_header->zero,
                    sizeof(olsr_u8_t));
  RoutingOLSRv2_Niigata_RotateShiftByte((olsr_u8_t *)&p_header->packet_semantics,
                    sizeof(olsr_u8_t));
  RoutingOLSRv2_Niigata_RotateShiftByte((olsr_u8_t *)&p_header->packet_seq_num,
                    sizeof(olsr_u16_t));
  new_msg += sizeof(struct packet_header);

  memcpy(new_msg, msg, msg_size);

  new_msg -= sizeof(struct packet_header);

  return new_msg;
}

static
olsr_u8_t *
RoutingOLSRv2_Niigata_EncapsulateMessage(Node *node,
                     olsr_u8_t *msg,
                     olsr_u32_t msg_size,
                     olsr_u32_t *new_msg_size,
                     unsigned char index)
{
    struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

    olsr_u8_t* buf = (olsr_u8_t*)MEM_malloc(MAXMESSAGESIZE);
    memset(buf, 0, MAXMESSAGESIZE);

    olsr_u8_t* buf_ptr = buf;
    olsr_u32_t extra_size = 0;

    *new_msg_size = 0;

    if (msg != NULL && msg_size > 0){
    if (msg_size % 4 != 0)
    {
        extra_size = 4 - (msg_size % 4);
    }
    if ((msg_size  + extra_size + sizeof(struct packet_header)) <= MAXMESSAGESIZE)
    {
        *new_msg_size = *new_msg_size + msg_size + extra_size;
        memcpy(buf_ptr, msg, msg_size);
        buf_ptr = buf_ptr +(msg_size + extra_size);
    }
    }

    if (check_forward_infomation(olsr, index))
    {
    OLSR_LIST_ENTRY *tmp = NULL, *del = NULL;

    tmp = olsr->forward_info[index].msg_list.head;
    while (tmp)
    {
        extra_size = 0;
        if (tmp->size % 4 != 0)
        {
        extra_size = 4 - (tmp->size % 4);
        }
        if ((*new_msg_size + tmp->size + extra_size + sizeof(struct packet_header)) <=  MAXMESSAGESIZE)
        {
        *new_msg_size = *new_msg_size + (tmp->size + extra_size);

        memcpy(buf_ptr, tmp->data, tmp->size);
        buf_ptr += tmp->size + extra_size;

        del = tmp;
        tmp = tmp->next;

        OLSR_DeleteListEntry(&olsr->forward_info[index].msg_list, del);
        }
        else
        {
        tmp = tmp->next;
        }
    }
    }

    //setting value of *buf_ptr to NULL before leaving
    buf_ptr = NULL;

    return buf;
}

static void
RoutingOLSRv2_Niigata_Send(Node *node,
               olsr_u8_t *msg,
               olsr_u32_t packetsize,
               clocktype delay,
               unsigned char index)
{
    struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;
    olsrd_config* olsr_cnf = olsr->olsr_cnf;

    olsr_u8_t *new_msg = NULL;
    olsr_u32_t new_msg_size = 0;

    new_msg = RoutingOLSRv2_Niigata_AddPacketHeader(node,
                            msg,
                            packetsize,
                            &new_msg_size);

    NetworkType networkType;

    if (olsr_cnf->ip_version == AF_INET) {
        networkType = NETWORK_IPV4;
    }
    else
    {
        networkType = NETWORK_IPV6;
    }

    if (NetworkIpGetUnicastRoutingProtocolType(node, index, networkType)
    == ROUTING_PROTOCOL_OLSRv2_NIIGATA)
    {
    if (olsr_cnf->ip_version == AF_INET)
    {
        Message *udpmsg = APP_UdpCreateMessage(
            node,
            NetworkIpGetInterfaceAddress(node, index),
            APP_ROUTING_OLSRv2_NIIGATA,
            ANY_DEST,
            APP_ROUTING_OLSRv2_NIIGATA,
            TRACE_OLSRv2_NIIGATA,
            IPTOS_PREC_INTERNETCONTROL);

        APP_UdpSetOutgoingInterface(udpmsg, index);
        
        APP_AddPayload(
            node,
            udpmsg,
            new_msg,
            new_msg_size);
            
        APP_UdpSend(node, udpmsg, delay);
    }
    else
    {
        Address source_address, any_addr;

        source_address.networkType = NETWORK_IPV6;
        NetworkGetInterfaceInfo(node, index, &source_address, NETWORK_IPV6);

        //Multicast Cast address (ff0e::1)
        /*
          any_addr.networkType = NETWORK_IPV6;
          any_addr.interfaceAddr.ipv6.s6_addr32[0] = 3839;
          any_addr.interfaceAddr.ipv6.s6_addr32[1] = 0;
          any_addr.interfaceAddr.ipv6.s6_addr32[2] = 0;
          any_addr.interfaceAddr.ipv6.s6_addr32[3] = 16777216;
        */

        any_addr.interfaceAddr.ipv6.s6_addr8[0] = 0xff;
        any_addr.interfaceAddr.ipv6.s6_addr8[1] = 0x0e;
        any_addr.interfaceAddr.ipv6.s6_addr8[2] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[3] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[4] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[5] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[6] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[7] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[8] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[9] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[10] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[11] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[12] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[13] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[14] = 0x00;
        any_addr.interfaceAddr.ipv6.s6_addr8[15] = 0x01;
        any_addr.networkType = NETWORK_IPV6;

        Message *udpmsg = APP_UdpCreateMessage(
            node,
            source_address,
            APP_ROUTING_OLSRv2_NIIGATA,
            any_addr,
            APP_ROUTING_OLSRv2_NIIGATA,
            TRACE_OLSRv2_NIIGATA,
            IPTOS_PREC_INTERNETCONTROL);
            
        APP_UdpSetOutgoingInterface(udpmsg, index);
        
        APP_AddPayload(
            node,
            udpmsg,
            new_msg,
            new_msg_size);
            
        APP_UdpSend(node, udpmsg, delay);

    }

    olsr->stat.numMessageSent++;
    if (DEBUG_OLSRV2)
    {
        olsr_printf("Increment Message Send Count to %d.\n", olsr->stat.numMessageSent);
    }
    olsr->stat.sendMsgSize += new_msg_size;
    if (DEBUG_OLSRV2)
    {
        olsr_printf("Increment Message Size Count %lld.\n", olsr->stat.sendMsgSize);
    }
    }//if routing protocol olsrv2

    MEM_free(new_msg);
}

static void
RoutingOLSRv2_Niigata_ForwardMessage(Node *node, unsigned char index)
{
  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

  olsr_u8_t *encapsulated_msg = NULL;
  olsr_u32_t encapsulated_msg_size = 0;
  clocktype jitter;

  if (check_forward_infomation(olsr, olsr->interfaceMappingQualnet[index])){

    encapsulated_msg =
      RoutingOLSRv2_Niigata_EncapsulateMessage(node,
                           NULL,
                           0,
                           &encapsulated_msg_size,
                           olsr->interfaceMappingQualnet[index]);


    jitter = (clocktype) (RANDOM_nrand(olsr->seed) %
              (clocktype) olsr->qual_cnf->max_hello_jitter);

    RoutingOLSRv2_Niigata_Send(node,
                   encapsulated_msg,
                   encapsulated_msg_size,
                   jitter,
                   index);

    MEM_free(encapsulated_msg);
  }
}

static void
RoutingOLSRv2_Niigata_GenerateHelloMessage(Node *node,
                       clocktype helloInterval,
                       unsigned char index)
{
  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

  Message    *helloPeriodicMsg = NULL;
  olsr_u8_t *encapsulated_msg = NULL;
  olsr_u32_t encapsulated_msg_size = 0;
  clocktype jitter;
  olsr_pktbuf_t *pktbuf = NULL;

  pktbuf = olsr_pktbuf_alloc();
  olsr_pktbuf_clear(pktbuf);

  if (DEBUG_OLSRV2)
  {
      olsr_printf("Going to build Hello Messgae.\n");
  }

  build_hello_msg(olsr, pktbuf, &olsr->iface_addr[olsr->interfaceMappingQualnet[index]]);

  olsr->stat.numHelloGenerated++;

  if (DEBUG_OLSRV2)
  {
      olsr_printf("Increment Hello Count to %d.\n",olsr->stat.numHelloGenerated);
  }
  encapsulated_msg =
    RoutingOLSRv2_Niigata_EncapsulateMessage(node,
                         pktbuf->data,
                         (olsr_u32_t)pktbuf->len,
                         &encapsulated_msg_size,
                         olsr->interfaceMappingQualnet[index]);

  jitter = (clocktype) (RANDOM_nrand(olsr->seed) %
            (clocktype) olsr->qual_cnf->max_hello_jitter);

  //send hello message
  RoutingOLSRv2_Niigata_Send(node,
                 encapsulated_msg,
                 encapsulated_msg_size,
                 jitter,
                 index);

  //set next event
  helloPeriodicMsg = MESSAGE_Alloc(node,
                   APP_LAYER,
                   APP_ROUTING_OLSRv2_NIIGATA,
                   MSG_APP_OLSRv2_NIIGATA_PeriodicHello);

  MESSAGE_InfoAlloc(node, helloPeriodicMsg, sizeof(unsigned char));
  *(unsigned char* )MESSAGE_ReturnInfo(helloPeriodicMsg) = index;

  MESSAGE_Send(node, helloPeriodicMsg, helloInterval);

  MEM_free(encapsulated_msg);
  olsr_pktbuf_free(&pktbuf);
}

static void
RoutingOLSRv2_Niigata_GenerateTcMessage(Node *node,
                    clocktype tcInterval,
                    unsigned char index)
{
  struct olsrv2 *olsr =
      (struct olsrv2 *)node->appData.olsrv2;

  Message    *tcPeriodicMsg = NULL;
  olsr_u8_t *encapsulated_msg = NULL;
  olsr_u32_t  encapsulated_msg_size = 0;
  olsr_pktbuf_t *pktbuf = NULL;
  clocktype jitter;

  jitter = (clocktype) (RANDOM_nrand(olsr->seed) %
            (clocktype) olsr->qual_cnf->max_tc_jitter);

  if (olsr->I_am_MPR_flag){
  //if (olsr->adv_neigh_set.head != NULL){
      pktbuf = olsr_pktbuf_alloc();
      olsr_pktbuf_clear(pktbuf);

      if (DEBUG_OLSRV2){
      olsr_printf("Going to build TC Message.\n");
      }
      build_tc_msg(olsr, pktbuf, &olsr->iface_addr[olsr->interfaceMappingQualnet[index]]);
      olsr->stat.numTcGenerated++;

      if (DEBUG_OLSRV2){
      olsr_printf("Increment TC Count to %d.\n",olsr->stat.numTcGenerated);
      }

      encapsulated_msg =
      RoutingOLSRv2_Niigata_EncapsulateMessage(node,
                           pktbuf->data,
                           (olsr_u32_t)pktbuf->len,
                           &encapsulated_msg_size,
                           olsr->interfaceMappingQualnet[index]);

      //send tc message
      RoutingOLSRv2_Niigata_Send(node,
                 encapsulated_msg,
                 encapsulated_msg_size,
                 jitter,
                 index);

      MEM_free(encapsulated_msg);
      olsr_pktbuf_free(&pktbuf);

  }

  //set next event
  tcPeriodicMsg = MESSAGE_Alloc(node,
                APP_LAYER,
                APP_ROUTING_OLSRv2_NIIGATA,
                MSG_APP_OLSRv2_NIIGATA_PeriodicTc);

  MESSAGE_InfoAlloc(node, tcPeriodicMsg, sizeof(unsigned char));
  *(unsigned char* )MESSAGE_ReturnInfo(tcPeriodicMsg) = index;

  MESSAGE_Send(node, tcPeriodicMsg, tcInterval);
}

static void
RoutingOLSRv2_Niigata_InitQualnetRoute(Node *node)
{
  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;
  olsrd_config* olsr_cnf = olsr->olsr_cnf;
  int hash_index;

  Address* dest_addr = NULL;
  Address* next_addr = NULL;
  Address* i_addr = NULL;


  if (olsr_cnf->ip_version == AF_INET)
  {
      NetworkEmptyForwardingTable(node, ROUTING_PROTOCOL_OLSRv2_NIIGATA);
  }
  else
  {
      OLSR_LIST_ENTRY *tmp = NULL;
      OLSR_ROUTING_TUPLE *data = NULL;

      for (hash_index = 0; hash_index < HASHSIZE; hash_index++)
      {
      tmp = olsr->routing_set_old[hash_index].list.head;
      while (tmp)
      {
          data = (OLSR_ROUTING_TUPLE *)tmp->data;
          dest_addr = RoutingOLSRv2_Niigata_GetQualnetAddress(olsr, data->R_dest_iface_addr);
          next_addr = RoutingOLSRv2_Niigata_GetQualnetAddress(olsr, data->R_next_iface_addr);
          i_addr    = RoutingOLSRv2_Niigata_GetQualnetAddress(olsr, data->R_iface_addr);

          Ipv6UpdateForwardingTable
          (node,
           dest_addr->interfaceAddr.ipv6,
           data->R_prefix_length,
           next_addr->interfaceAddr.ipv6,
           Ipv6GetInterfaceIndexFromAddress
           (node,
            &i_addr->interfaceAddr.ipv6),
           NETWORK_UNREACHABLE,
           ROUTING_PROTOCOL_OLSRv2_NIIGATA);

          MEM_free(dest_addr);
          MEM_free(next_addr);
          MEM_free(i_addr);

          tmp = tmp->next;
      }
      }
  }
}

static void
RoutingOLSRv2_Niigata_FinalizeQualnetRoute(Node *node)
{
    struct olsrv2 *olsr = (struct olsrv2 *)node->appData.olsrv2;
    olsrd_config* olsr_cnf = olsr->olsr_cnf;

    if (olsr_cnf->ip_version == AF_INET)
    {
    RoutingOLSRv2_Niigata_InitQualnetRoute(node);
    }
    else
    {
    NetworkDataIp *networkDataIp = node->networkData.networkVar;
    route_init(networkDataIp->ipv6);
    }
}

static void
RoutingOLSRv2_Niigata_PrintQualnetRoutingTable(Node *node)
{
    struct olsrv2* olsr = (struct olsrv2 *)node->appData.olsrv2;
    olsrd_config* olsr_cnf = olsr->olsr_cnf;



    if (olsr_cnf->ip_version == AF_INET)
    {
    NetworkPrintForwardingTable(node);
    }
    else if (olsr_cnf->ip_version == AF_INET6)
    {
    Ipv6PrintForwardingTable(node);
    }
    else
    {
    olsr_error("%s: (%d) Un known ip_version.", __func__, __LINE__);
    }
}

static void
RoutingOLSRv2_Niigata_QualnetRoute(Node *node)
{
  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;
  olsrd_config* olsr_cnf = olsr->olsr_cnf;

  Address* dest_addr = NULL;
  Address* next_addr = NULL;
  Address* i_addr = NULL;


  OLSR_LIST_ENTRY *tmp = NULL;
  OLSR_ROUTING_TUPLE *data = NULL;
  int hash_index;

   RoutingOLSRv2_Niigata_InitQualnetRoute(node);

  if (olsr_cnf->ip_version == AF_INET6)
  {
      for (hash_index = 0; hash_index < HASHSIZE; hash_index++)
      {
      OLSR_DeleteList_Static(&olsr->routing_set_old[hash_index].list);
      OLSR_InitList(&olsr->routing_set_old[hash_index].list);
      OLSR_CopyList(&olsr->routing_set_old[hash_index].list, &olsr->routing_set[hash_index].list);
      }
  }

  for (hash_index = 0; hash_index < HASHSIZE; hash_index++)
  {
      tmp = olsr->routing_set[hash_index].list.head;
      while (tmp)
      {
      data = (OLSR_ROUTING_TUPLE *)tmp->data;

      dest_addr = RoutingOLSRv2_Niigata_GetQualnetAddress(olsr, data->R_dest_iface_addr);
      next_addr = RoutingOLSRv2_Niigata_GetQualnetAddress(olsr, data->R_next_iface_addr);
      i_addr = RoutingOLSRv2_Niigata_GetQualnetAddress(olsr, data->R_iface_addr);

      NodeAddress mask = 0;

      if (data->R_prefix_length == 32)
      {
          mask = 0xffffffff;
      }
      else if (data->R_prefix_length == 24)
      {
          mask = 0xffffff00;
      }
      else if (data->R_prefix_length == 16)
      {
          mask = 0xffff0000;
      }
      else if (data->R_prefix_length == 8)
      {
          mask = 0xff000000;
      }
      else if (data->R_prefix_length == 0)
      {
          mask = 0x00000000;
      }
      else
      {
          for (int i=0; i<data->R_prefix_length; i++)
          {
          mask += 1 << (31 - i);
          }
      }

      if (olsr_cnf->ip_version == AF_INET)
      {
          NetworkUpdateForwardingTable
          (node,
           dest_addr->interfaceAddr.ipv4,
           mask,
           next_addr->interfaceAddr.ipv4,
           NetworkIpGetInterfaceIndexFromAddress(node,i_addr->interfaceAddr.ipv4),
           data->R_dist,
           ROUTING_PROTOCOL_OLSRv2_NIIGATA);
      }
      else if (olsr_cnf->ip_version == AF_INET6)
      {

          Ipv6UpdateForwardingTable
          (node,
           dest_addr->interfaceAddr.ipv6,
           data->R_prefix_length,
           next_addr->interfaceAddr.ipv6,
           Ipv6GetInterfaceIndexFromAddress
           (node,
            &i_addr->interfaceAddr.ipv6),
           data->R_dist,
           ROUTING_PROTOCOL_OLSRv2_NIIGATA);
      }
      else
      {
          olsr_error("%s: (%d) Un known ip_version.", __func__, __LINE__);
      }

      MEM_free(dest_addr);
      MEM_free(next_addr);
      MEM_free(i_addr);

      tmp = tmp->next;
      }
  }

  if (DEBUG_OLSRV2)
  {
     //Commented to avoid printing forwarding tables on screen
     //RoutingOLSRv2_Niigata_PrintQualnetRoutingTable(node);
  }
}

static void
RoutingOLSRv2_Niigata_CheckTimeoutTuples(Node *node)
{
  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

  Message    *timeoutTuplesMsg = NULL;

  olsr_check_timeout_tuples(olsr);

  olsr_process_changes(olsr);

  if (olsr->changes_neighborhood || olsr->changes_topology)
  {
      RoutingOLSRv2_Niigata_QualnetRoute(node);
  }

  //flush changes flags
  olsr->changes_neighborhood = OLSR_FALSE;
  olsr->changes_topology = OLSR_FALSE;


  //set next event
  timeoutTuplesMsg = MESSAGE_Alloc(node,
                   APP_LAYER,
                   APP_ROUTING_OLSRv2_NIIGATA,
                   MSG_APP_OLSRv2_NIIGATA_TimeoutTuples);


  MESSAGE_Send(node, timeoutTuplesMsg, (clocktype) olsr->qual_cnf->timeout_interval);
}


static void
RoutingOLSRv2_Niigata_UpdateCurrentSimTime(Node *node)
{
  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

  olsr->current_sim_time = node->getNodeTime();
}

static void
RoutingOLSRv2_Niigata_HandlePacket(Node *node, Message *msg)
{

  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

  //NON-OLSRv2 node received OLSRv2 packet
  if (olsr == NULL)
  {
      return;
  }

  olsr_u8_t *packet = NULL;
  olsr_u16_t packet_size;
  UdpToAppRecv *udpToApp = NULL;
  union olsr_ip_addr *source_addr = NULL;
  union olsr_ip_addr saddr;
  int incomingInterface;

  olsrd_config* olsr_cnf = olsr->olsr_cnf;

  packet = (olsr_u8_t *) MESSAGE_ReturnPacket(msg);
  packet_size = (olsr_u16_t) MESSAGE_ReturnPacketSize(msg);

  udpToApp = (UdpToAppRecv *) MESSAGE_ReturnInfo(msg);

  incomingInterface = udpToApp->incomingInterfaceIndex;

  NetworkType networkType;

  if (olsr_cnf->ip_version == AF_INET) {
     networkType = NETWORK_IPV4;
  }
  else
  {
     networkType = NETWORK_IPV6;
  }

  if (NetworkIpGetUnicastRoutingProtocolType(node,incomingInterface,networkType)
      != ROUTING_PROTOCOL_OLSRv2_NIIGATA)
  {
      return;
  }

  source_addr =
    RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, udpToApp->sourceAddr);
  memcpy(&saddr, source_addr, sizeof(union olsr_ip_addr));

   //now parse packet
  olsr->parsingIfNum = olsr->interfaceMappingQualnet[incomingInterface];

  assert(olsr->parsingIfNum != OLSR_INVALID_INTERFACE);

  if (DEBUG_OLSRV2)
  {
      char* paddr =
      olsr_niigata_ip_to_string(olsr, &olsr->iface_addr[olsr->parsingIfNum]);
      olsr_printf("Receiving node iface_addr = %s\n", paddr);
      free(paddr);
      paddr = olsr_niigata_ip_to_string(olsr, source_addr);
      olsr_printf("Receiving Message source_addr = %s\n", paddr);
      free(paddr);

      RoutingOLSRv2_Niigata_DumpPacket((char *)packet,packet_size);
  }

  olsr->stat.recvMsgSize += packet_size;
  if (DEBUG_OLSRV2)
  {
      olsr_printf("Increment Received Msg Size to %lld.\n",olsr->stat.recvMsgSize);
  }
  RoutingOLSRv2_Niigata_UpdateCurrentSimTime(node);
  parse_packet(olsr,
           (char *)packet,
           packet_size,
           &olsr->iface_addr[olsr->parsingIfNum],
           saddr);

  olsr_process_changes(olsr);

  if (olsr->changes_neighborhood || olsr->changes_topology)
  {
      RoutingOLSRv2_Niigata_QualnetRoute(node);
  }

  //flush changes flags
  olsr->changes_neighborhood = OLSR_FALSE;
  olsr->changes_topology = OLSR_FALSE;

  int iface_num;
  for (iface_num=0; iface_num < olsr->iface_num; iface_num++)
  {
      if (olsr->forwardMsg[iface_num] == NULL &&
     check_forward_infomation(olsr, (unsigned char)iface_num))
      {
      Message *timeoutForwardMsg;

      clocktype delay =
          (clocktype) (RANDOM_nrand(olsr->seed) % olsr->qual_cnf->max_forward_jitter);

      timeoutForwardMsg = MESSAGE_Alloc(node,
                        APP_LAYER,
                        APP_ROUTING_OLSRv2_NIIGATA,
                        MSG_APP_OLSRv2_NIIGATA_TimeoutForward);

      MESSAGE_InfoAlloc(node, timeoutForwardMsg, sizeof(unsigned char));
      *(unsigned char* )MESSAGE_ReturnInfo(timeoutForwardMsg) = olsr->interfaceMappingOlsr[iface_num];
      olsr->forwardMsg[iface_num] = (void* )timeoutForwardMsg;

      MESSAGE_Send(node, timeoutForwardMsg, delay);
      }
  }

  MEM_free(source_addr);
}


void
RoutingOLSRv2_Niigata_LinkLayerNotificationHandler
(Node *node,
 const Message *msg,
 const NodeAddress nextHopAddress,
 const int interfaceIndex)
{
  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;
  olsrd_config* olsr_cnf = olsr->olsr_cnf;

  char* paddr;

  Address qualnet_addr;
  memset(&qualnet_addr, 0, sizeof(Address));

  if (DEBUG_OLSRV2)
  {
      olsr_printf("%s: Link Layer Notification!!\n", __FUNCTION__);
  }

  if (olsr_cnf->ip_version == AF_INET)
  {
      qualnet_addr.networkType = NETWORK_IPV4;
      qualnet_addr.interfaceAddr.ipv4 = nextHopAddress;
  }
  else
  {
      olsr_error("Does not support IPv6\n");
  }

  if (DEBUG_OLSRV2)
  {
      union olsr_ip_addr* next_addr =
      RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, qualnet_addr);
      paddr = olsr_niigata_ip_to_string(olsr, next_addr);
      olsr_printf("Link Break is IP(%s)\n",paddr);
      MEM_free(paddr);
      MEM_free(next_addr);
  }


  RoutingOLSRv2_Niigata_UpdateCurrentSimTime(node);

  union olsr_ip_addr* addr1 = RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, qualnet_addr);
  union olsr_ip_addr  saddr1;

  memcpy(&saddr1, addr1, sizeof(union olsr_ip_addr));

  proc_link_layer_notification(olsr, saddr1);

  if (DEBUG_OLSRV2)
  {
      print_mpr_set(olsr);
      print_routing_set(olsr);
  }

  if (olsr->changes_neighborhood || olsr->changes_topology)
  {
      RoutingOLSRv2_Niigata_QualnetRoute(node);
  }

  if (DEBUG_OLSRV2)
  {
     //Commented to avoid printing forwarding tables on screen
      // RoutingOLSRv2_Niigata_PrintQualnetRoutingTable(node);
  }

  //flush changes flags
  olsr->changes_neighborhood = OLSR_FALSE;
  olsr->changes_topology = OLSR_FALSE;

  if (olsr->qual_cnf->packet_restoration)
  {
      IpHeaderType* ipHeader;
      Address address;
      union olsr_ip_addr* source_addr;
      union olsr_ip_addr* dest_addr;

      if (DEBUG_OLSRV2)
      {
      olsr_printf("%s: Packet Restration!!\n", __FUNCTION__);
      }
      ipHeader = (IpHeaderType *) MESSAGE_ReturnPacket(msg);
      address.networkType = NETWORK_IPV4;
      address.interfaceAddr.ipv4 = ipHeader->ip_src;
      source_addr =
      RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, address);

      address.interfaceAddr.ipv4 = ipHeader->ip_dst;
      dest_addr =
      RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, address);

      if (DEBUG_OLSRV2)
      {
      paddr = olsr_niigata_ip_to_string(olsr, &olsr->main_addr);
      olsr_printf("main_addr = %s\n", paddr);
      MEM_free(paddr);
      paddr = olsr_niigata_ip_to_string(olsr, source_addr);
      olsr_printf("source_addr = %s\n", paddr);
      MEM_free(paddr);
      paddr = olsr_niigata_ip_to_string(olsr, dest_addr);
      olsr_printf("dest_addr = %s\n", paddr);
      MEM_free(paddr);
      }

      switch(olsr->qual_cnf->restoration_type)
      {
      case 1: //Simple
      {
          Message *restMsg = MESSAGE_Duplicate(node, msg);

          if (DEBUG_OLSRV2)
          {
          olsr_printf("%s: Simple Restration!!\n", __FUNCTION__);
          }
#ifdef ADDON_DB
            HandleNetworkDBEvents(
                node,
                restMsg,
                interfaceIndex,                
                "NetworkReceiveFromUpper",
                "",
                ipHeader->ip_src,
                ipHeader->ip_dst,
                IpHeaderGetTOS(ipHeader->ip_v_hl_tos_len),
                ipHeader->ip_p);
#endif
          RouteThePacketUsingLookupTable(node, restMsg, interfaceIndex);
          break;
      }

      case 2: //Full
      {
          ERROR_Assert(OLSR_FALSE, "Not Support Full Restration...");
          break;
      }

      default:
      {
          ERROR_Assert(OLSR_FALSE, "No Such Restration Type...");
          break;
      }
      }

      MEM_free(source_addr);
      MEM_free(dest_addr);
  }

  MEM_free(addr1);
}


static void
OLSRv2InitConfigParameters(Node* node, const NodeInput* nodeInput, int interfaceIndex)
{
    BOOL wasFound;
    char buf[MAX_STRING_LENGTH];
    char errStr[MAX_STRING_LENGTH];

    struct olsrv2 *olsr = (struct olsrv2 *)node->appData.olsrv2;
    olsrd_config* olsr_cnf = olsr->olsr_cnf;

    OLSRv2QualConf *qual_cnf =
      (OLSRv2QualConf *)MEM_malloc(sizeof(OLSRv2QualConf));
    memset(qual_cnf,0,sizeof(OLSRv2QualConf));

    olsr->qual_cnf = qual_cnf;


    if (olsr_cnf->ip_version == AF_INET)
    {

        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-HELLO-INTERVAL",
            &wasFound,
            (clocktype *)&qual_cnf->hello_interval);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadTime(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-HELLO-INTERVAL",
            &wasFound,
            (clocktype *)&qual_cnf->hello_interval);
    }

    if (!wasFound)
    {
 /*       sprintf(errStr, "Hello Interval Parameter not specified"
                        " for Node %d, hence continuing with the"
                        " default value: %dS. \n",
                        node->nodeId, (int)(OLSRv2_DEFAULT_HELLO_INTERVAL / SECOND) );
        ERROR_ReportWarning(errStr);*/

        qual_cnf->hello_interval = OLSRv2_DEFAULT_HELLO_INTERVAL;
    }
    else
    {
        if (qual_cnf->hello_interval <= 0 )
        {
           sprintf(errStr, "Invalid Hello Interval Parameter specified"
                            " for Node %d, hence continuing"
                            " with the default value: %dS. \n",
                            node->nodeId, (int)(OLSRv2_DEFAULT_HELLO_INTERVAL / SECOND) );
            ERROR_ReportWarning(errStr);

            qual_cnf->hello_interval = OLSRv2_DEFAULT_HELLO_INTERVAL;
        }
    }

    if (olsr_cnf->ip_version == AF_INET)
    {

        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-TC-INTERVAL",
            &wasFound,
            (clocktype *)&qual_cnf->tc_interval);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadTime(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-TC-INTERVAL",
            &wasFound,
            (clocktype *)&qual_cnf->tc_interval);
    }

    if (!wasFound)
    {
  /*      sprintf(errStr, "TC Interval Parameter not specified"
                        " for Node %d, hence continuing with the"
                        " default value: %dS. \n",
                        node->nodeId, (int)(OLSRv2_DEFAULT_TC_INTERVAL / SECOND));
        ERROR_ReportWarning(errStr);*/
        qual_cnf->tc_interval = OLSRv2_DEFAULT_TC_INTERVAL;
    }
    else
    {
        if (qual_cnf->tc_interval <= 0 )
        {
            sprintf(errStr, "Invalid TC Interval Parameter specified"
                            " for Node %d, hence continuing"
                            " with the default value: %dS. \n",
                            node->nodeId, (int)(OLSRv2_DEFAULT_TC_INTERVAL / SECOND));
            ERROR_ReportWarning(errStr);

            qual_cnf->tc_interval = OLSRv2_DEFAULT_TC_INTERVAL;
        }
    }


    if (olsr_cnf->ip_version == AF_INET)
    {

        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-REFRESH-TIMEOUT-INTERVAL",
            &wasFound,
            (clocktype *)&qual_cnf->timeout_interval);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadTime(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-REFRESH-TIMEOUT-INTERVAL",
            &wasFound,
            (clocktype *)&qual_cnf->timeout_interval);
    }

    if (!wasFound)
    {
  /*      sprintf(errStr, "Timeout Interval Parameter not specified"
                " for Node %d, hence continuing with the"
                " default value: %dS. \n",
                node->nodeId, (int)(OLSRv2_DEFAULT_REFRESH_TIMEOUT_INTERVAL / SECOND));
        ERROR_ReportWarning(errStr);*/

        qual_cnf->timeout_interval = OLSRv2_DEFAULT_REFRESH_TIMEOUT_INTERVAL;
    }
    else
    {
        if (qual_cnf->timeout_interval <= 0 )
        {
            sprintf(errStr, "Invalid Timeout Interval Parameter specified"
                            " for Node %d, hence continuing"
                            " with the default value: %dS. \n",
                            node->nodeId,
                            (int)(OLSRv2_DEFAULT_REFRESH_TIMEOUT_INTERVAL / SECOND));
            ERROR_ReportWarning(errStr);

            qual_cnf->timeout_interval = OLSRv2_DEFAULT_REFRESH_TIMEOUT_INTERVAL;
        }
    }


    //setting Hello jitter
    qual_cnf->max_hello_jitter = OLSRv2_DEFAULT_MAX_HELLO_JITTER;

    //setting TC jitter
    qual_cnf->max_tc_jitter = OLSRv2_DEFAULT_MAX_TC_JITTER;

    //setting Forward jitter
    qual_cnf->max_forward_jitter = OLSRv2_DEFAULT_MAX_FORWARD_JITTER;


    if (olsr_cnf->ip_version == AF_INET)
    {
        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-START-HELLO",
            &wasFound,
            (clocktype *)&qual_cnf->start_hello);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadTime(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-START-HELLO",
            &wasFound,
            (clocktype *)&qual_cnf->start_hello);
    }

    if (!wasFound)
    {
  /*      sprintf(errStr, "Start Hello Interval Parameter not specified"
                " for Node %d, hence continuing with the"
                " default value: %dS. \n",
                node->nodeId, (int)(OLSRv2_DEFAULT_START_HELLO / SECOND));
        ERROR_ReportWarning(errStr);*/

        qual_cnf->start_hello = OLSRv2_DEFAULT_START_HELLO;
    }
    else
    {
        if (qual_cnf->start_hello <= 0 )
        {
            sprintf(errStr, "Invalid Start Hello Interval Parameter specified"
                            " for Node %d, hence continuing"
                            " with the default value: %dS. \n",
                            node->nodeId, (int)(OLSRv2_DEFAULT_START_HELLO / SECOND));
            ERROR_ReportWarning(errStr);

            qual_cnf->start_hello = OLSRv2_DEFAULT_START_HELLO;
        }
    }


    if (olsr_cnf->ip_version == AF_INET)
    {

        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-START-TC",
            &wasFound,
            (clocktype *)&qual_cnf->start_tc);
    }

    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

         IO_ReadTime(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-START-TC",
            &wasFound,
            (clocktype *)&qual_cnf->start_tc);
    }

    if (!wasFound)
    {
   /*     sprintf(errStr, "Start TC Interval Parameter not specified"
                " for Node %d, hence continuing with the"
                " default value: %dS. \n",
                node->nodeId, (int)(OLSRv2_DEFAULT_START_TC / SECOND));
        ERROR_ReportWarning(errStr);*/

        qual_cnf->start_tc = OLSRv2_DEFAULT_START_TC;
    }
    else
    {
        if (qual_cnf->start_tc <= 0 )
        {
            sprintf(errStr, "Invalid Start TC Interval Parameter specified"
                            " for Node %d, hence continuing"
                            " with the default value: %dS. \n",
                            node->nodeId, (int)(OLSRv2_DEFAULT_START_TC / SECOND));
            ERROR_ReportWarning(errStr);

            qual_cnf->start_tc = OLSRv2_DEFAULT_START_TC;
        }
    }


    if (olsr_cnf->ip_version == AF_INET)
    {

        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-START-REFRESH-TIMEOUT",
            &wasFound,
            (clocktype *)&qual_cnf->start_timeout);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadTime(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-START-REFRESH-TIMEOUT",
            &wasFound,
            (clocktype *)&qual_cnf->start_timeout);
    }

    if (!wasFound)
    {
  /*      sprintf(errStr, "Start Timeout Interval Parameter not specified"
                " for Node %d, hence continuing with the"
                " default value: %dS. \n",
                node->nodeId, (int)(OLSRv2_DEFAULT_START_REFRESH_TIMEOUT / SECOND));
        ERROR_ReportWarning(errStr);*/

        qual_cnf->start_timeout = OLSRv2_DEFAULT_START_REFRESH_TIMEOUT;
    }
    else
    {
        if (qual_cnf->start_timeout <= 0 )
        {
            sprintf(errStr, "Invalid Start Timeout Interval Parameter specified"
                            " for Node %d, hence continuing"
                            " with the default value: %dS. \n",
                            node->nodeId,
                            (int)(OLSRv2_DEFAULT_START_REFRESH_TIMEOUT / SECOND));
            ERROR_ReportWarning(errStr);

            qual_cnf->start_timeout = OLSRv2_DEFAULT_START_REFRESH_TIMEOUT;
        }
    }

    wasFound = FALSE;

    if (olsr_cnf->ip_version == AF_INET)
    {

        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-NEIGHBOR-HOLD-TIME",
            &wasFound,
            (clocktype *)&qual_cnf->neighbor_hold_time);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadTime(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-NEIGHBOR-HOLD-TIME",
            &wasFound,
            (clocktype *)&qual_cnf->neighbor_hold_time);
    }

    if (!wasFound)
    {
        qual_cnf->neighbor_hold_time =
                                  OLSRv2_DEFAULT_NEIGHBOR_HOLD_TIME / SECOND;
    }
    else
    {

        qual_cnf->neighbor_hold_time = qual_cnf->neighbor_hold_time / SECOND;

        if (qual_cnf->neighbor_hold_time <= 0 )
        {
           sprintf(errStr, "Invalid Neighbor Hold Time specified"
                            " for Node %d, hence continuing"
                            " with the default value: %dS. \n",
                            node->nodeId,
                            (int)(OLSRv2_DEFAULT_NEIGHBOR_HOLD_TIME / SECOND));
            ERROR_ReportWarning(errStr);

            qual_cnf->neighbor_hold_time =
                                  OLSRv2_DEFAULT_NEIGHBOR_HOLD_TIME / SECOND;
        }
    }


    if (olsr_cnf->ip_version == AF_INET)
    {

        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-TOPOLOGY-HOLD-TIME",
            &wasFound,
            (clocktype *)&qual_cnf->topology_hold_time);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadTime(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-TOPOLOGY-HOLD-TIME",
            &wasFound,
            (clocktype *)&qual_cnf->topology_hold_time);
    }

    if (!wasFound)
    {
        qual_cnf->topology_hold_time =
                                  OLSRv2_DEFAULT_TOPOLOGY_HOLD_TIME / SECOND;
    }
    else
    {

        qual_cnf->topology_hold_time = qual_cnf->topology_hold_time / SECOND;

        if (qual_cnf->topology_hold_time <= 0 )
        {
           sprintf(errStr, "Invalid Topology Hold Time specified"
                            " for Node %d, hence continuing"
                            " with the default value: %dS. \n",
                            node->nodeId,
                            (int)(OLSRv2_DEFAULT_TOPOLOGY_HOLD_TIME / SECOND));
            ERROR_ReportWarning(errStr);

            qual_cnf->topology_hold_time =
                                  OLSRv2_DEFAULT_TOPOLOGY_HOLD_TIME / SECOND;
        }
    }


    if (olsr_cnf->ip_version == AF_INET)
    {

        IO_ReadTime(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-DUPLICATE-HOLD-TIME",
            &wasFound,
            (clocktype *)&qual_cnf->duplicate_hold_time);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadTime(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-DUPLICATE-HOLD-TIME",
            &wasFound,
            (clocktype *)&qual_cnf->duplicate_hold_time);
    }

    if (!wasFound)
    {
        qual_cnf->duplicate_hold_time =
                                 OLSRv2_DEFAULT_DUPLICATE_HOLD_TIME / SECOND;
    }
    else
    {
        qual_cnf->duplicate_hold_time = qual_cnf->duplicate_hold_time /SECOND;

        if (qual_cnf->duplicate_hold_time <= 0 )
        {
           sprintf(errStr, "Invalid Duplicate Hold Time specified"
                            " for Node %d, hence continuing"
                            " with the default value: %dS. \n",
                            node->nodeId,
                            (int)(OLSRv2_DEFAULT_DUPLICATE_HOLD_TIME / SECOND));
            ERROR_ReportWarning(errStr);

            qual_cnf->duplicate_hold_time =
                                 OLSRv2_DEFAULT_DUPLICATE_HOLD_TIME / SECOND;
        }
    }


    if (olsr_cnf->ip_version == AF_INET)
    {
        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-LINK-LAYER-NOTIFICATION",
            &wasFound,
            buf);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadString(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-LINK-LAYER-NOTIFICATION",
            &wasFound,
            buf);
    }

    if (wasFound == TRUE)
      {
    if (strcmp(buf, "NO") == 0)
      {
        qual_cnf->link_layer_notification = OLSR_FALSE;
      }
    else if (strcmp(buf, "YES") == 0)
      {
        qual_cnf->link_layer_notification = OLSR_TRUE;
      }
    else
      {
          //ERROR_ReportError("Needs YES/NO against OLSRv2-LINK-LAYER-NOTIFICATION");
          sprintf(errStr, "Invalid value for Link Layer Notification"
              " specified for Node %d. Value should be"
              " either YES or NO. Hence"
              " continuing with the default value NO. \n",
              node->nodeId);
          ERROR_ReportWarning(errStr);

          qual_cnf->link_layer_notification =
                          (olsr_bool )OLSRv2_DEFAULT_LINK_LAYER_NOTIFICATION;
      }
      }
    else
      {
    qual_cnf->link_layer_notification =
      (olsr_bool )OLSRv2_DEFAULT_LINK_LAYER_NOTIFICATION;
      }


    if (olsr_cnf->ip_version == AF_INET)
    {
        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-PACKET-RESTORATION",
            &wasFound,
            buf);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadString(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-PACKET-RESTORATION",
            &wasFound,
            buf);
    }

    if (wasFound == TRUE)
      {
    if (strcmp(buf, "NO") == 0)
      {
        qual_cnf->packet_restoration = OLSR_FALSE;
      }
    else if (strcmp(buf, "YES") == 0)
      {
        qual_cnf->packet_restoration = OLSR_TRUE;
      }
    else
      {
          //ERROR_ReportError("Needs YES/NO against OLSRv2-PACKET-RESTORATION");
          sprintf(errStr, "Invalid Packet Restoration Type"
              " specified for Node %d. Restoration type should be"
              " either YES or NO. Hence"
              " continuing with the default value. \n",
              node->nodeId);
          ERROR_ReportWarning(errStr);

          qual_cnf->packet_restoration =
                                (olsr_bool)OLSRv2_DEFAULT_PACKET_RESTORATION;
      }
      }
    else
      {
    qual_cnf->packet_restoration = (olsr_bool)OLSRv2_DEFAULT_PACKET_RESTORATION;
      }

    if (qual_cnf->packet_restoration == TRUE)
      {
    if (olsr_cnf->ip_version == AF_INET)
      {
        IO_ReadInt(
               node->nodeId,
               NetworkIpGetInterfaceAddress(node, interfaceIndex),
               nodeInput,
               "OLSRv2-RESTORATION-TYPE",
               &wasFound,
               &qual_cnf->restoration_type);
      }
    else
      {
        Address address;

        NetworkGetInterfaceInfo(
                    node,
                    interfaceIndex,
                    &address,
                    NETWORK_IPV6);

        IO_ReadInt(
               node->nodeId,
               &address.interfaceAddr.ipv6,
               nodeInput,
               "OLSRv2-RESTORATION-TYPE",
               &wasFound,
               &qual_cnf->restoration_type);
      }

    if (wasFound == FALSE)
      {
        qual_cnf->restoration_type = OLSRv2_DEFAULT_RESTORATION_TYPE;
      }
    else
      {
        if (qual_cnf->restoration_type != 1){
          sprintf(errStr, "Invalid Restoration Type"
              " specified for Node %d. Restoration type other"
              " than SIMPLE is not supported. Hence"
              " continuing with the default value %d. \n",
              node->nodeId, (int)OLSRv2_DEFAULT_RESTORATION_TYPE);
          ERROR_ReportWarning(errStr);

          qual_cnf->restoration_type = OLSRv2_DEFAULT_RESTORATION_TYPE;
        }
      }

      }
}

static void
OLSRv2InitAttachedNetwork(Node* node,
                          const NodeInput* nodeInput,
                          int interfaceIndex)
{
    BOOL wasFound = FALSE;
    char buf[MAX_STRING_LENGTH];
    char* token = NULL;
    char errStr[MAX_STRING_LENGTH];
    char* next = NULL;
    char delims[] = {" \t"};
    char iotoken[MAX_STRING_LENGTH];
    struct olsrv2 *olsr = (struct olsrv2 *)node->appData.olsrv2;
    olsrd_config* olsr_cnf = olsr->olsr_cnf;
    NodeAddress nodeId = node->nodeId;
    ATTACHED_NETWORK *attachedNetwork, *iterator;
    BOOL duplicateFound = FALSE;

    //olsr->attached_net_addr = NULL;
    //olsr->advNetAddrNum = 0;

    if (olsr_cnf->ip_version == AF_INET)
    {
        IO_ReadString(
            nodeId,
            NetworkIpGetInterfaceAddress(node, interfaceIndex),
            nodeInput,
            "OLSRv2-ATTACHED-NETWORK",
            &wasFound,
            buf);
    }
    else
    {
        Address address;

        NetworkGetInterfaceInfo(
            node,
            interfaceIndex,
            &address,
            NETWORK_IPV6);

        IO_ReadString(
            node->nodeId,
            &address.interfaceAddr.ipv6,
            nodeInput,
            "OLSRv2-ATTACHED-NETWORK",
            &wasFound,
            buf);
    }

    if (wasFound)
    {
        token = IO_GetDelimitedToken(iotoken, buf, delims, &next);
        if (token != NULL)
        {
            olsr->I_am_MPR_flag = OLSR_TRUE;
            while (token != NULL)
            {
                attachedNetwork =
                    (ATTACHED_NETWORK* )MEM_malloc(sizeof(ATTACHED_NETWORK));
                memset(attachedNetwork, 0, sizeof(ATTACHED_NETWORK));

                GetIpAddr(olsr, &attachedNetwork->network_addr, token);

                token = IO_GetDelimitedToken(iotoken, next, delims, &next);
                if (token == NULL)
                {
                    sprintf(errStr, "Invalid Attached Network Configuration."
                                    " Configuration requires pair of"
                                    " Network Address and"
                                    " Network Prefix length.");
                    ERROR_Assert(OLSR_FALSE, errStr);
                    //assert(OLSR_FALSE);
                }
                attachedNetwork->prefix_length = (olsr_u8_t) atoi(token);

                iterator = olsr->attached_net_addr;
                while (iterator != NULL)
                {
                    if (equal_ip_addr((struct olsrv2 *)olsr,
                                        &iterator->network_addr,
                                        &attachedNetwork->network_addr))
                    {
                        duplicateFound = TRUE;
                        //duplicate entry found in configuration
                        //now checking for prefix length
                        if (attachedNetwork->prefix_length >
                                         iterator->prefix_length)
                        {
                            //updating prefix length
                            iterator->prefix_length =
                                             attachedNetwork->prefix_length;
                        }
                        break;
                    }
                    else
                    {
                        iterator = iterator->next;
                    }
                }//End of while (iterator !=NULL)

                if (!duplicateFound)
                {
                    attachedNetwork->next = olsr->attached_net_addr;
                    olsr->attached_net_addr = attachedNetwork;
                    olsr->advNetAddrNum++;
                }
                else
                {
                    free(attachedNetwork);
                    duplicateFound = FALSE;
                }

                token = IO_GetDelimitedToken(iotoken, next, delims, &next);
            }//End of while (token != NULL)
        }
    }
}


void
RoutingOLSRv2_Niigata_Init(Node *node,
                           const NodeInput* nodeInput,
                           int interfaceIndex,
                           NetworkType networkType)
{
  Message *timeoutMsg = NULL, *helloPeriodicMsg = NULL, *tcPeriodicMsg = NULL;

  char* paddr;

  struct olsrv2 *olsr =
    (struct olsrv2 *)MEM_malloc(sizeof(struct olsrv2));
  memset(olsr, 0, sizeof(struct olsrv2));

  node->appData.olsrv2 = olsr;
  olsrd_config* olsr_cnf = NULL;


  if (DEBUG_OUTPUT || DEBUG_OLSRV2)
  {
      //init olsr debug
      init_olsr_debug();
      olsr_printf("************************************\n");
      olsr_printf("initiate node %3d\n", node->nodeId);
  }

  //init olsr config
  olsr_cnf = (struct olsrd_config *)MEM_malloc(sizeof(struct olsrd_config));
  memset(olsr_cnf, 0, sizeof(struct olsrd_config));

  olsr->olsr_cnf = olsr_cnf;

  RANDOM_SetSeed(olsr->seed,
                 node->globalSeed,
                 node->nodeId,
                 APP_ROUTING_OLSRv2_NIIGATA);

    if (networkType == NETWORK_IPV4){
      olsr_cnf->ip_version = AF_INET;
      olsr_cnf->ipsize = sizeof(olsr_u32_t);

      if (DEBUG_OUTPUT || DEBUG_OLSRV2)
      {
      olsr_printf("IP version = 4, ipsize = %d\n", (int)olsr_cnf->ipsize);
      }
    }
    else if (networkType == NETWORK_IPV6){
      olsr_cnf->ip_version = AF_INET6;
      olsr_cnf->ipsize = sizeof(in6_addr);

      if (DEBUG_OUTPUT || DEBUG_OLSRV2)
      {
      olsr_printf("IP version = 6, ipsize = %d\n", (int)olsr_cnf->ipsize);
      }
  }
  else
  {
      olsr_error("Unknown IP type\n");
  }

  OLSRv2InitConfigParameters(node, nodeInput, interfaceIndex);

  //init olsr debug level parameter by 0
  olsr_cnf->debug_level = 0;
  //init tuples
  olsr_init_tuples(olsr);
  //init message seqno
  init_msg_seqno(olsr);
  //init packet seqno
  init_packet_seqno(olsr);
  //init parser
  olsr_init_parser(olsr);
  //init parse address
  init_parse_address(olsr);

  union olsr_ip_addr* m_addr = NULL;
  //init orignator address
  if (olsr_cnf->ip_version == AF_INET){
      Address address;
      address.networkType = NETWORK_IPV4;
      address.interfaceAddr.ipv4 =
      NetworkIpGetInterfaceAddress(node, interfaceIndex);

      m_addr = RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, address);

      memcpy(&olsr->main_addr, m_addr, sizeof(union olsr_ip_addr));
      MEM_free(m_addr);
  }
  else
  {
      Address address;
      address.networkType = NETWORK_IPV6;

      NetworkGetInterfaceInfo(node, interfaceIndex, &address, NETWORK_IPV6);

      m_addr = RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, address);

      memcpy(&olsr->main_addr, m_addr, sizeof(union olsr_ip_addr));
      MEM_free(m_addr);
  }

  if (DEBUG_OUTPUT || DEBUG_OLSRV2)
  {
      paddr = olsr_niigata_ip_to_string(olsr, &olsr->main_addr);
      olsr_printf("main_addr = %s\n",paddr);
      MEM_free(paddr);
  }

  union olsr_ip_addr* i_addr = NULL;
  int numberInterfaces = node->numberInterfaces;

  //init interface address
  olsr->iface_num = 1;
  olsr->iface_addr =
    (union olsr_ip_addr *)MEM_malloc
    (numberInterfaces*sizeof(union olsr_ip_addr));
  olsr->forward_info =
      (struct forward_infomation* )MEM_malloc
      (numberInterfaces*sizeof(struct forward_infomation));
  memset(olsr->forward_info, 0,
     numberInterfaces*sizeof(struct forward_infomation));
  olsr->forwardMsg = (void** )MEM_malloc(numberInterfaces*sizeof(void* ));
  memset(olsr->forwardMsg, 0, numberInterfaces*sizeof(void* ));

  //interfaces mapping
  olsr->interfaceMappingOlsr =
    (olsr_u8_t* )MEM_malloc(sizeof(olsr_u8_t)*numberInterfaces);
  memset(olsr->interfaceMappingOlsr, 0, sizeof(olsr_u8_t)*numberInterfaces);
  olsr->interfaceMappingQualnet =
    (olsr_u8_t* )MEM_malloc(sizeof(olsr_u8_t)*numberInterfaces);
  memset(olsr->interfaceMappingQualnet, 0, sizeof(olsr_u8_t)*numberInterfaces);

  for (int i=0; i<numberInterfaces; i++)
  {
    olsr->interfaceMappingOlsr[i] = OLSR_INVALID_INTERFACE;
    olsr->interfaceMappingQualnet[i] = OLSR_INVALID_INTERFACE;
  }

  if (olsr_cnf->ip_version == AF_INET)
  {
      Address address;
      address.networkType = NETWORK_IPV4;
      address.interfaceAddr.ipv4 =
    NetworkIpGetInterfaceAddress(node, interfaceIndex);

      i_addr = RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, address);
      memcpy(&olsr->iface_addr[0], i_addr, sizeof(union olsr_ip_addr));
      MEM_free(i_addr);
  }
  else
  {
      Address address;
      address.networkType = NETWORK_IPV6;
      NetworkGetInterfaceInfo(node, interfaceIndex, &address, NETWORK_IPV6);

      i_addr = RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, address);
      memcpy(&olsr->iface_addr[0], i_addr, sizeof(union olsr_ip_addr));
      MEM_free(i_addr);
  }

  olsr->interfaceMappingQualnet[interfaceIndex] = 0;
  olsr->interfaceMappingOlsr[0] = (olsr_u8_t) interfaceIndex;


  if (DEBUG_OUTPUT || DEBUG_OLSRV2)
  {

      paddr = olsr_niigata_ip_to_string(olsr, &olsr->iface_addr[0]);
      olsr_printf("index = %d, iface_addr = %s\n", interfaceIndex, paddr);
      free(paddr);
      olsr_printf("************************************\n\n");
  }

  //init sim time
  RoutingOLSRv2_Niigata_UpdateCurrentSimTime(node);
  //init qualnet route
  RoutingOLSRv2_Niigata_InitQualnetRoute(node);

  //init link layer notification
  if (olsr->qual_cnf->link_layer_notification)
  {
      if (olsr_cnf->ip_version == AF_INET)
      {
    NetworkIpSetMacLayerStatusEventHandlerFunction
      (node,
       RoutingOLSRv2_Niigata_LinkLayerNotificationHandler,
       interfaceIndex);
      }
      else
      {
      olsr_error("Link Layer Notification is not supported for IPv6!!\n");
      }
  }

  olsr->attached_net_addr = NULL;
  olsr->advNetAddrNum = 0;
  OLSRv2InitAttachedNetwork(node, nodeInput, interfaceIndex);

  //set timeout event
  timeoutMsg = MESSAGE_Alloc(node,
                 APP_LAYER,
                 APP_ROUTING_OLSRv2_NIIGATA,
                 MSG_APP_OLSRv2_NIIGATA_TimeoutTuples);
  MESSAGE_Send(node, timeoutMsg, (clocktype) olsr->qual_cnf->start_timeout);

  //set hello message send  event
  helloPeriodicMsg = MESSAGE_Alloc(node,
                   APP_LAYER,
                   APP_ROUTING_OLSRv2_NIIGATA,
                   MSG_APP_OLSRv2_NIIGATA_PeriodicHello);

  MESSAGE_InfoAlloc(node, helloPeriodicMsg, sizeof(unsigned char));
  *(unsigned char* )MESSAGE_ReturnInfo(helloPeriodicMsg) =
                                           (unsigned char)interfaceIndex;
  MESSAGE_Send(node,helloPeriodicMsg,(clocktype)olsr->qual_cnf->start_hello);

  //set tc message send event
  tcPeriodicMsg = MESSAGE_Alloc(node,
                APP_LAYER,
                APP_ROUTING_OLSRv2_NIIGATA,
                MSG_APP_OLSRv2_NIIGATA_PeriodicTc);

  MESSAGE_InfoAlloc(node, tcPeriodicMsg, sizeof(unsigned char));
  *(unsigned char* )MESSAGE_ReturnInfo(tcPeriodicMsg) =
                                           (unsigned char)interfaceIndex;
  MESSAGE_Send(node,tcPeriodicMsg,(clocktype) olsr->qual_cnf->start_tc);
}

void
RoutingOLSRv2_Niigata_AddInterface(Node *node,
                                   const NodeInput* nodeInput,
                                   int interfaceIndex,
                                   NetworkType networkType)
{
  struct olsrv2 *olsr = (struct olsrv2 *)node->appData.olsrv2;
  olsrd_config* olsr_cnf = olsr->olsr_cnf;

   if ((olsr_cnf->ip_version == AF_INET && networkType == NETWORK_IPV6)
       ||
       (olsr_cnf->ip_version == AF_INET6 && networkType == NETWORK_IPV4))
   {
     ERROR_ReportError("Both IPv4 and IPv6 instance of"
      "OLSRv2-NIIGATA can not run simultaneously on a node");
   }

  union olsr_ip_addr* i_addr = NULL;

  Message *helloPeriodicMsg = NULL, *tcPeriodicMsg = NULL;
  char* paddr = NULL;

  olsr->iface_num++;

  if (olsr_cnf->ip_version == AF_INET)
  {
      Address address;
      address.networkType = NETWORK_IPV4;
      address.interfaceAddr.ipv4 =
    NetworkIpGetInterfaceAddress(node, interfaceIndex);

      i_addr = RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, address);
      memcpy(&olsr->iface_addr[olsr->iface_num-1], i_addr, sizeof(union olsr_ip_addr));
      MEM_free(i_addr);
  }
  else
  {
      Address address;
      address.networkType = NETWORK_IPV6;
      NetworkGetInterfaceInfo(node, interfaceIndex, &address, NETWORK_IPV6);

      i_addr = RoutingOLSRv2_Niigata_GetOlsrv2_Niigata_Address(olsr, address);
      memcpy(&olsr->iface_addr[olsr->iface_num-1], i_addr, sizeof(union olsr_ip_addr));
      MEM_free(i_addr);
  }

  olsr->interfaceMappingQualnet[interfaceIndex] = olsr->iface_num-1;
  olsr->interfaceMappingOlsr[olsr->iface_num-1] = (olsr_u8_t)interfaceIndex;

  if (DEBUG_OUTPUT || DEBUG_OLSRV2)
  {
    paddr = olsr_niigata_ip_to_string(olsr, &olsr->iface_addr[olsr->iface_num-1]);
    olsr_printf("index = %d, iface_addr = %s\n", interfaceIndex, paddr);
    free(paddr);
    olsr_printf("************************************\n\n");
  }

  //init attach network
  OLSRv2InitAttachedNetwork(node, nodeInput, interfaceIndex);

  //init link layer notification
  if (olsr->qual_cnf->link_layer_notification)
  {
      if (olsr_cnf->ip_version == AF_INET)
      {
    NetworkIpSetMacLayerStatusEventHandlerFunction
      (node,
       RoutingOLSRv2_Niigata_LinkLayerNotificationHandler,
       interfaceIndex);
      }
      else
      {
      olsr_error("Link Layer Notification: Not Suport IPv6!!\n");
      }
  }

  //set hello message send  event
  helloPeriodicMsg = MESSAGE_Alloc(node,
                   APP_LAYER,
                   APP_ROUTING_OLSRv2_NIIGATA,
                   MSG_APP_OLSRv2_NIIGATA_PeriodicHello);

  MESSAGE_InfoAlloc(node, helloPeriodicMsg, sizeof(unsigned char));
  *(unsigned char* )MESSAGE_ReturnInfo(helloPeriodicMsg) =
                                               (unsigned char)interfaceIndex;
  MESSAGE_Send(node,helloPeriodicMsg,(clocktype)olsr->qual_cnf->start_hello);

  //set tc message send event
  tcPeriodicMsg = MESSAGE_Alloc(node,
                APP_LAYER,
                APP_ROUTING_OLSRv2_NIIGATA,
                MSG_APP_OLSRv2_NIIGATA_PeriodicTc);

  MESSAGE_InfoAlloc(node, tcPeriodicMsg, sizeof(unsigned char));
  *(unsigned char* )MESSAGE_ReturnInfo(tcPeriodicMsg) =
                                              (unsigned char) interfaceIndex;
  MESSAGE_Send(node, tcPeriodicMsg, (clocktype) olsr->qual_cnf->start_tc);
}

static
void RoutingOLSRv2_Niigata_PrintStat(Node *node)
{
    struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

    olsr_stat stat;
    char buf[MAX_STRING_LENGTH];

    stat = olsr->stat;

    if (node->appData.routingStats)
    {

        sprintf(buf, "Hello Messages Received = %u",
           stat.numHelloReceived);
        IO_PrintStat(
        node,
        "Application",
        "OLSRv2_NIIGATA",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

        sprintf(buf, "Hello Messages Sent = %u",
            stat.numHelloGenerated);
        IO_PrintStat(
            node,
        "Application",
        "OLSRv2_NIIGATA",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

        sprintf(buf, "TC Messages Received = %u",
           stat.numTcReceived);
        IO_PrintStat(
            node,
        "Application",
        "OLSRv2_NIIGATA",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

        sprintf(buf, "TC Messages Generated = %u",
            stat.numTcGenerated);
        IO_PrintStat(
        node,
        "Application",
        "OLSRv2_NIIGATA",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

        sprintf(buf, "TC Messages Relayed = %u",
           stat.numTcRelayed);
        IO_PrintStat(
            node,
        "Application",
        "OLSRv2_NIIGATA",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

        sprintf(buf, "Total Messages Received = %u",
            stat.numMessageRecv);
        IO_PrintStat(
        node,
        "Application",
        "OLSRv2_NIIGATA",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

        sprintf(buf, "Total Messages Sent = %u",
            stat.numMessageSent);
        IO_PrintStat(
        node,
        "Application",
        "OLSRv2_NIIGATA",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

        sprintf(buf, "Total Bytes of Messages Received = %" TYPES_64BITFMT "d",
            stat.recvMsgSize);
        IO_PrintStat(
        node,
        "Application",
        "OLSRv2_NIIGATA",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

        sprintf(buf, "Total Bytes of Messages Sent = %" TYPES_64BITFMT "d",
           stat.sendMsgSize);
        IO_PrintStat(
        node,
        "Application",
        "OLSRv2_NIIGATA",
        ANY_DEST,
        -1 /* instance Id */,
        buf);

    }


}

void
RoutingOLSRv2_Niigata_Finalize(Node* node, int ifNum)
{
    struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

    if (!olsr->stat.statsPrinted)
    {
        RoutingOLSRv2_Niigata_PrintStat(node);
        olsr->stat.statsPrinted = OLSR_TRUE;

        if (DEBUG_OUTPUT || DEBUG_OLSRV2)
        {
            olsr_printf("\nFor Node Id: %d\n",node->nodeId);
            print_link_set(olsr);
            print_2neigh_set(olsr);
            print_mpr_set(olsr);
            print_mpr_selector_set(olsr);
            print_adv_neigh_set(olsr);
            print_assn_history_set(olsr);
            print_attached_set(olsr);
            print_relay_set(olsr);
            print_forward_set(olsr);
            print_proc_set(olsr);
            print_rcv_msg_set(olsr);
            print_topology_set(olsr);
            print_routing_set(olsr);
        }
    }
}


void
RoutingOLSRv2_Niigata_Layer(Node *node, Message *msg)
{
  struct olsrv2 *olsr =
    (struct olsrv2 *)node->appData.olsrv2;

  unsigned char index;
  char* paddr = NULL;

  switch (msg->eventType)
  {
      case MSG_APP_FromTransport:
      {
      RoutingOLSRv2_Niigata_HandlePacket(node, msg);

      MESSAGE_Free(node, msg);
      break;
      }

      case MSG_APP_OLSRv2_NIIGATA_PeriodicHello:
      {
      RoutingOLSRv2_Niigata_UpdateCurrentSimTime(node);

      if (DEBUG_OLSRV2)
      {
          paddr = olsr_niigata_ip_to_string(olsr, &olsr->main_addr);
          olsr_printf("Node %d with address %s encounter Hello Message Generation Event.\n",
             node->nodeId, paddr);
          MEM_free(paddr);
      }

      olsrd_config* olsr_cnf = olsr->olsr_cnf;

      NetworkType networkType;

      if (olsr_cnf->ip_version == AF_INET) {
         networkType = NETWORK_IPV4;
      }
      else
      {
         networkType = NETWORK_IPV6;
      }

      index = *(unsigned char* )MESSAGE_ReturnInfo(msg);
      // Do not consider  non-olsrv2 interfaces
      if (NetworkIpGetUnicastRoutingProtocolType(node, index, networkType)
          == ROUTING_PROTOCOL_OLSRv2_NIIGATA)
      {
          RoutingOLSRv2_Niigata_GenerateHelloMessage(
          node,
          (clocktype)olsr->qual_cnf->hello_interval,
          index);
      }

      MESSAGE_Free(node, msg);

      break;
      }

      case MSG_APP_OLSRv2_NIIGATA_PeriodicTc:
      {
      RoutingOLSRv2_Niigata_UpdateCurrentSimTime(node);

      if (DEBUG_OLSRV2)
      {
          paddr = olsr_niigata_ip_to_string(olsr, &olsr->main_addr);
          olsr_printf("Node %d with address %s encounter TC Message Generation Event.\n",
             node->nodeId, paddr);
          MEM_free(paddr);
      }

      olsrd_config* olsr_cnf = olsr->olsr_cnf;
      NetworkType networkType;

      if (olsr_cnf->ip_version == AF_INET) {
         networkType = NETWORK_IPV4;
      }
      else
      {
         networkType = NETWORK_IPV6;
      }

      index = *(unsigned char* )MESSAGE_ReturnInfo(msg);
      // Do not consider  non-olsrv2 interfaces
      if (NetworkIpGetUnicastRoutingProtocolType(node, index, networkType)
          == ROUTING_PROTOCOL_OLSRv2_NIIGATA)
      {
          RoutingOLSRv2_Niigata_GenerateTcMessage(
          node,
          (clocktype)olsr->qual_cnf->tc_interval,
          index);
      }

      MESSAGE_Free(node, msg);
      break;
      }

      case MSG_APP_OLSRv2_NIIGATA_TimeoutForward:
      {
      RoutingOLSRv2_Niigata_UpdateCurrentSimTime(node);

      if (DEBUG_OLSRV2)
      {
          paddr = olsr_niigata_ip_to_string(olsr, &olsr->main_addr);
          olsr_printf("Node %d with address %s encounter Timeout Event for Fowarding Messages.\n",
             node->nodeId, paddr);
          MEM_free(paddr);
      }

      olsrd_config* olsr_cnf = olsr->olsr_cnf;
      NetworkType networkType;

      if (olsr_cnf->ip_version == AF_INET) {
         networkType = NETWORK_IPV4;
      }
      else
      {
         networkType = NETWORK_IPV6;
      }

      index = *(unsigned char* )MESSAGE_ReturnInfo(msg);
      if (NetworkIpGetUnicastRoutingProtocolType(node, index, networkType)
          == ROUTING_PROTOCOL_OLSRv2_NIIGATA)
      {
          RoutingOLSRv2_Niigata_ForwardMessage(node, index);
          olsr->forwardMsg[index] = NULL;
      }

      MESSAGE_Free(node, msg);
      break;
      }

      case MSG_APP_OLSRv2_NIIGATA_TimeoutTuples:
      {
      RoutingOLSRv2_Niigata_UpdateCurrentSimTime(node);

      if (DEBUG_OLSRV2)
      {
          paddr = olsr_niigata_ip_to_string(olsr, &olsr->main_addr);
          olsr_printf("Node %d with address %s encounter Timeout Event for Tuple Validation.\n",
             node->nodeId, paddr);
          free(paddr);
      }

      RoutingOLSRv2_Niigata_CheckTimeoutTuples(node);

      MESSAGE_Free(node, msg);
      break;
      }

      default:
      {
      /* invalid message */
      ERROR_Assert(OLSR_FALSE, "Invalid switch value");
      break;
      }
  }
}
