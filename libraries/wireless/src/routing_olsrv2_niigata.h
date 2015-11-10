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
#ifndef _ADDON_OLSRv2_NIIGATA_
#define _ADDON_OLSRv2_NIIGATA_


//olsrv2_niigata

//qualnet
//#include "main.h"

#define OLSRv2_DEFAULT_HELLO_INTERVAL             (2 * SECOND) //(SECOND)
#define OLSRv2_DEFAULT_TC_INTERVAL                (5 * SECOND) //(SECOND)

#define OLSRv2_DEFAULT_REFRESH_TIMEOUT_INTERVAL   (2 * SECOND) //(SECOND)

#define OLSRv2_DEFAULT_MAX_HELLO_JITTER         (250 * MILLI_SECOND) //(MILLISECOND)
#define OLSRv2_DEFAULT_MAX_TC_JITTER            (250 * MILLI_SECOND) //(MILLISECOND)

#define OLSRv2_DEFAULT_MAX_FORWARD_JITTER       (250 * MILLI_SECOND)//(MILLISECOND)

#define OLSRv2_DEFAULT_START_HELLO             (1 * SECOND) //(SECOND)
#define OLSRv2_DEFAULT_START_TC                (1 * SECOND) //(SECOND)
#define OLSRv2_DEFAULT_START_REFRESH_TIMEOUT   (1 * SECOND) //(SECOND)

#define OLSRv2_DEFAULT_NEIGHBOR_HOLD_TIME      (3 * OLSRv2_DEFAULT_HELLO_INTERVAL) //(SECOND)
#define OLSRv2_DEFAULT_TOPOLOGY_HOLD_TIME      (3 * OLSRv2_DEFAULT_TC_INTERVAL) //(SECOND)
#define OLSRv2_DEFAULT_DUPLICATE_HOLD_TIME    (30 * SECOND) //(SECOND)

#define OLSRv2_DEFAULT_LINK_LAYER_NOTIFICATION  FALSE
#define OLSRv2_DEFAULT_PACKET_RESTORATION       TRUE
#define OLSRv2_DEFAULT_RESTORATION_TYPE         1 //1:Simple

/* Shift to olsr_conf.h for MAC mismatch
typedef enum
  {
    L_ENDIAN,
    B_ENDIAN
  }MACHINE_TYPE;
*/
void RoutingOLSRv2_Niigata_Init(Node *node,
                                const NodeInput *nodeInput,
                                int interfaceIndex,
                                NetworkType networkType);
void
RoutingOLSRv2_Niigata_AddInterface(Node *node,
                                   const NodeInput *nodeInput,
                                   int interfaceIndex,
                                   NetworkType networkType);
void RoutingOLSRv2_Niigata_Layer(Node *, Message *);
void RoutingOLSRv2_Niigata_Finalize(Node* , int);
#endif
